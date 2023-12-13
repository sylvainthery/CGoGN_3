/*******************************************************************************
 * CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
 * Copyright (C), IGG Group, ICube, University of Strasbourg, France            *
 *                                                                              *
 * This library is free software; you can redistribute it and/or modify it      *
 * under the terms of the GNU Lesser General Public License as published by the *
 * Free Software Foundation; either version 2.1 of the License, or (at your     *
 * option) any later version.                                                   *
 *                                                                              *
 * This library is distributed in the hope that it will be useful, but WITHOUT  *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
 * for more details.                                                            *
 *                                                                              *
 * You should have received a copy of the GNU Lesser General Public License     *
 * along with this library; if not, write to the Free Software Foundation,      *
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
 *                                                                              *
 * Web site: http://cgogn.unistra.fr/                                           *
 * Contact information: cgogn@unistra.fr                                        *
 *                                                                              *
 *******************************************************************************/

#include <cgogn/rendering/shaders/shader_ao_shadows_post.h>
#include<cgogn/ui/view.h>
#include <cgogn/rendering/shadows.h>


using ::cgogn::rendering::GLVec4d;
using ::cgogn::rendering::GLMat4d;

namespace cgogn
{

namespace rendering
{

ShaderFullScreenHBAO* ShaderFullScreenHBAO::instance_ = nullptr;


ShaderFullScreenHBAO::ShaderFullScreenHBAO()
{
	const char* vertex_shader_source = R"(
		#version 330
		out vec2 tc;

		
		void main()
		{
			tc = vec2(gl_VertexID % 2, gl_VertexID / 2);
			vec2 stc = 2.0 * tc - 1.0;
			gl_Position = vec4(stc, 0.0, 1.0);
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		in vec2 tc;

		out float frag_out;

		uniform sampler2D TUdepth;
		uniform sampler2D TUnormal;
		uniform vec2 projv;  // [ projection_matrix[0][0],projection_matrix.data[1][1] ];
		uniform vec3 fn;    // [ -2.0*zfar*znearn, zfar+znear, zfar-znear ];
		uniform float radius;
		#define  nb_dirs 7
		#define nb_steps 7
		#define amb_ratio 0.3

		uniform float ao_strength;
		uniform vec3 light_position;
		uniform mat4 shadow_matrix;
		uniform vec2 bias_k;
		uniform vec2 bias_adapt;
		uniform sampler2D TUshadow;
		uniform sampler2D TUpoisson;
		uniform int nb_samples;
		uniform vec4 plane;


		#define TWO_PI  6.28318531
		#define PI      3.141592654
		#define PIOV2      3.141592654


		//  ~sin(5°)
		#define ANGLE_BIAS 0.87155

		float rand1Dp(vec2 coord, int i) //generating noise/pattern texture for dithering
		{
			float dp = dot(vec3(coord,3.721*float(i+1)), vec3(12.9898,78.233,45.164));
			return fract(sin(dp) * 43758.5453);
		}

		vec3 pos_from_depth(vec2 uv, float d)
		{
			float z_s = 2.0 * d - 1.0;
			float z = fn.x / (fn.y - z_s * (fn.z));
			return vec3(-(2.0*uv-1.0)/projv, 1.0) * z;
		}

		vec3 pos_from_ZB(vec2 uv)
		{
			float d = texture(TUdepth,uv).r;
			return pos_from_depth(uv,d);
		}

		vec3 min_len_vect(vec3 P, vec3 Pr, vec3 Pl)
		{
		  vec3 V1 = Pr - P;
		  vec3 V2 = P - Pl;
		  return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
		}

		vec3 N_from_ZB(in vec2 uv, in vec3 P)
		{
			vec2 ts =  vec2(1)/textureSize(TUdepth,0);
			vec3 Pr = pos_from_ZB(uv+vec2(ts.x,0.0));
			vec3 Pl = pos_from_ZB(uv+vec2(-ts.x,0.0));
			vec3 Pb = pos_from_ZB(uv+vec2(0.0,ts.y));
			vec3 Pt = pos_from_ZB(uv+vec2(0.0,-ts.y));
			return normalize(cross(min_len_vect(P, Pr, Pl), min_len_vect(P, Pb, Pt)));
		}


	mat2 createRot(float theta)
	{
		float cosTheta = cos(theta);
		float sinTheta = sin(theta);
		return mat2(cosTheta, -sinTheta, sinTheta, cosTheta);
	}

		
	float compute_hbao(vec3 P, vec3 N)
	{
		vec2 projectedRad = - radius/2.0 * projv / P.z;
		vec2 screenRadius = textureSize(TUdepth,0) * projectedRad;
		if (dot(screenRadius,screenRadius) < 25)
			return 0.0;

		mat2 deltaRotationMatrix = createRot(TWO_PI / float(nb_dirs));
		mat2 MR = createRot( rand1Dp(tc,0)*PIOV2);
		vec2 deltaUV = MR * vec2(screenRadius.x/float(nb_steps), 0.0);

		float occlusion = 0.0;

		vec2 inv_d_wh = vec2(1)/textureSize(TUdepth,0);
		for(int i = 0; i < nb_dirs; i++)
		{
			deltaUV = deltaRotationMatrix * deltaUV;
			vec2 sampleUV = gl_FragCoord.xy + (1.0+rand1Dp(tc,i))*deltaUV;
			float occ_d = 0.0;
			for(int j = 0; j < nb_steps; j++)
			{
				vec2 fcoo = (sampleUV+float(j)*deltaUV);
				ivec2 icoo = ivec2(fcoo);
				float depth = texelFetch(TUdepth,icoo,0).x;
				if (depth < 1.0)
				{
					vec3 sampleVS = pos_from_depth(fcoo*inv_d_wh, depth);
					vec3 sampleDirVS = sampleVS - P;
					float sampleLength = length(sampleDirVS);
//					float occ = (max(ANGLE_BIAS,dot(N, sampleDirVS)/sampleLength)-ANGLE_BIAS)/(1.0-ANGLE_BIAS);
					float occ = max(0.0,dot(N, sampleDirVS)/sampleLength);
					float att = max(0.0,1.0-(length(sampleDirVS.xy) / radius));
					occ_d = max(occ_d, occ * att);
				}
			}

			occlusion += occ_d;
		}
		return occlusion / float(nb_dirs);
	}


		float random(vec3 seed,int i)
		{
			float dp = dot(vec4(seed,3.721*float(i)), vec4(12.9898,78.233,45.164,17.371));
			return fract(sin(dp) * 43758.5453);
		}

		float shadowTexTest(vec3 scoo, float dnl)
		{
			float d = texture(TUshadow, scoo.xy).r;
			float bias = (bias_adapt.x + bias_adapt.y * d) * bias_k.x * (2.0-dnl);
			return ((d > (scoo.z-bias))?1.0:0.0);
		}

		float compute_shadow_map(vec3 P, vec3 N)
		{
			vec3 L = normalize(light_position - P);
			float dnl = dot(N,L);
			vec4 sh_coord = shadow_matrix*vec4(P,1);
			sh_coord.xyz /= sh_coord.w;
			
			sh_coord.z = clamp(sh_coord.z,0.001,0.999);

			float sc = 1.5/textureSize(TUshadow,0).x;

			float shad = shadowTexTest(sh_coord.xyz, dnl);

			for (int i=1;i<nb_samples;i++)
			{
				int index = int(15.99*random(gl_FragCoord.xyz,i));
				vec3 shc =	vec3(sh_coord.xy + texelFetch(TUpoisson,ivec2(index,0),0).xy*sc,sh_coord.z - 0.001 * random(gl_FragCoord.xyz,i+64));
				shad += shadowTexTest(shc,dnl); 
			}
			return shad/float(nb_samples);
		}

		void main()
		{
			float depth = texture(TUdepth,tc).r;
			if (depth>=1.0)
			{
				vec2 stc = 2.0*tc -1.0;
				float q = dot(plane.xyz,vec3(-stc/projv,1));
				if (abs(q)>0.0001)
				{
					float z = plane.w/q;
					vec3 P = vec3(-stc/projv, 1.0) * z;
					frag_out = compute_shadow_map(P,plane.xyz);
				}
				else
					frag_out = 1.0;
			}
			else
			{
				vec3 P = pos_from_depth(tc,depth);
				// N * 2 -1 because storage is RGB8
				vec3 N = normalize(textureLod(TUnormal,tc,0.0).rgb*2.0-1.0);
				float hbao = max(0.0, 0.9999 - compute_hbao(P,N)*ao_strength); // 0.9999 for the fract in final shader path
				float shadow = amb_ratio + (1.0-amb_ratio) * compute_shadow_map(P,N);
				frag_out = hbao * shadow;
			}
		}
	)";

	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("TUdepth", "TUnormal",	"projv", "fn", "radius", "ao_strength",
		"shadow_matrix", "TUshadow", "bias_k", "bias_adapt","TUpoisson", "nb_samples", "plane");


}

ShaderParamFullScreenHBAO::ShaderParamFullScreenHBAO(ShaderType* sh):
	ShaderParam(sh), tex_d_(nullptr), radius_(0.08f), ao_strength_(1.6f),
	light_position_(10, 100, 1000)
{
}


void ShaderParamFullScreenHBAO::set_uniforms()
{	
	shader_->set_uniforms_values(tex_d_->bind(0), tex_n_->bind(1), projv_, fn_, radius_,
								 ao_strength_,
								 shadataptr_->shadow_matrix_,
								 shadataptr_->fbo_shadows_->getDepthTexture()->bind(2), shadataptr_->bias_k_,
								 shadataptr_->bias_adapt_, shadataptr_->tex_poisson_->bind(3),
								 shadataptr_->nb_samples_, plane_);
}


void ShaderParamFullScreenHBAO::draw(::cgogn::ui::View* v)
{
	const GLMat4d& mproj = v->projection_matrix_d();
	float64 znear = mproj(2, 3) / (mproj(2, 2) - 1.0);
	float64 zfar = mproj(2, 3) / (mproj(2, 2) + 1.0);

	projv_ = ::cgogn::rendering::GLVec2(mproj(0, 0), mproj(1, 1));
	fn_ = ::cgogn::rendering::GLVec3(float32(- 2.0 * zfar * znear), float32(zfar + znear), float32(zfar - znear));

	bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	release();
}


ShaderFullScreenApplyHBAO* ShaderFullScreenApplyHBAO::instance_ = nullptr;

ShaderParamFullScreenApplyHBAO::ShaderParamFullScreenApplyHBAO(ShaderType* sh)
	: ShaderParam(sh), tex_ambiant_(nullptr), tex_diffuse_(nullptr)
{
}

ShaderFullScreenApplyHBAO::ShaderFullScreenApplyHBAO()
{
	const char* vertex_shader_source = R"(
		#version 330
		out vec2 tc;
		void main()
		{
			tc = vec2(gl_VertexID % 2, gl_VertexID / 2);
			gl_Position = vec4(2.0 * tc - 1.0, 0.0, 1.0);
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		in vec2 tc;
		out vec3 frag_out;

		uniform sampler2D TUambiant;
		uniform sampler2D TUdiffuse;

		void main()
		{
			vec3 diff = texture(TUdiffuse,tc).rgb;
			float ao_shadow = texture(TUambiant,tc).r ;
			frag_out = diff * ao_shadow;
		}
	)";

	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("TUambiant", "TUdiffuse");
}

void ShaderParamFullScreenApplyHBAO::set_uniforms()
{
	shader_->set_uniforms_values(tex_ambiant_->bind(0), tex_diffuse_->bind(1));
}

void ShaderParamFullScreenApplyHBAO::draw()
{
	bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	release();
}



ShaderFSBlurAO* ShaderFSBlurAO::instance_ = nullptr;

ShaderFSBlurAO::ShaderFSBlurAO()
{
	const char* vertex_shader_source = R"(
		#version 330
		out vec2 tc;
		void main()
		{
			tc = vec2(gl_VertexID % 2, gl_VertexID / 2);
			gl_Position = vec4(2.0 * tc - 1.0, 0.0, 1.0);
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		in vec2 tc;
		out float frag_out;

		uniform sampler2D TU;
		uniform ivec2 dtx;

// 0.335568, 0.241812, 0.090404
		void main()
		{
			ivec2 txc = ivec2(gl_FragCoord.xy);
			float v0 = texelFetch(TU,txc,0).r;
			float v1 = texelFetch(TU,txc-dtx,0).r + texelFetch(TU,txc+dtx,0).r;
			float texel = (v0>v1) ? 0.5 * v1 : 0.44198 * v0 + 0.27901 * v1 ;
			frag_out = texel;
			//frag_out = 0.44198 * v0 + 0.27901 * v1 ;
		}
	)";

	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("TU", "dtx");
}

void ShaderParamFSBlurAO::set_uniforms()
{
	shader_->set_uniforms_values(tex_->bind(0), dtx_);
}

void ShaderParamFSBlurAO::blurH()
{
	dtx_ = {1, 0};
	bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	release();
}

void ShaderParamFSBlurAO::blurV()
{
	dtx_ = {0, 1}; 
	bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	release();
}


} // namespace rendering

} // namespace cgogn
