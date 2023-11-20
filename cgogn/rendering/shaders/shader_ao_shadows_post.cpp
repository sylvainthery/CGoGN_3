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
			tc = 2.0* vec2(gl_VertexID % 2, gl_VertexID / 2);
			vec2 stc = 2.0 * tc - 1.0;
			gl_Position = vec4(stc, 0.0, 1.0);
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		in vec2 tc;

		out float frag_out;

		uniform sampler2D TUdepth;
		uniform vec2 projv;  // [ projection_matrix[0][0],projection_matrix.data[1][1] ];
		uniform vec3 fn;    // [ -2.0*zfar*znearn, zfar+znear, zfar-znear ];
		uniform float radius;
		uniform float subs;
		uniform int nb_dirs;
		uniform int nb_steps;
		uniform float time;
		uniform float ao_strength;
		uniform vec3 light_position;
		uniform mat4 shadow_matrix;
		uniform vec2 bias_k;
		uniform sampler2DShadow TUshadow;
		uniform sampler2D TUpoisson;
		uniform int nb_samples;
		uniform float plane_p;
		uniform vec3 plane_n;
		uniform mat4 invMat;

		#define TWO_PI  6.28318531
		#define PI      3.141592654

		// sin(PI/48)
		#define ANGLE_BIAS 0.13	

		float rand1Dp(vec2 coord) //generating noise/pattern texture for dithering
		{
			return fract(sin(dot(coord ,vec2(12.9898,78.233))*time) * 43758.5453);
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

		
	float compute_hbao(vec3 P, vec3 N)
	{
		float projectedRad = - radius/2.0 * projv.x / P.z;
		float screenRadius = projectedRad * textureSize(TUdepth,0).x;
		if (screenRadius < 3.0)
		{
			return 0.0;
		}

		float theta = TWO_PI / float(nb_dirs);
		float cosTheta = cos(theta);
		float sinTheta = sin(theta);
		mat2 deltaRotationMatrix = mat2(cosTheta, -sinTheta, sinTheta, cosTheta);

		float angl1 = rand1Dp(tc)*TWO_PI;
		float cosThetaR = cos(angl1);
		float sinThetaR = sin(angl1);
		mat2 MR = mat2(cosThetaR, -sinThetaR, sinThetaR, cosThetaR);
		vec2 deltaUV = MR * vec2(screenRadius/float(nb_steps), 0.0);

		float occlusion = 0.0;

		vec2 inv_d_wh = vec2(1)/textureSize(TUdepth,0); //  ?? /sub ?

		for(int i = 0; i < nb_dirs; i++)
		{
			deltaUV = deltaRotationMatrix * deltaUV;
			vec2 sampleUV = gl_FragCoord.xy + (0.5+rand1Dp(tc))*deltaUV;
			float occ_d = 0.0;
			for(int j = 0; j < nb_steps; j++)
			{
				vec2 fcoo = (sampleUV+float(j)*deltaUV);
				ivec2 icoo = ivec2(fcoo*subs);
				float depth = texelFetch(TUdepth,icoo,0).x;
				if (depth < 1.0)
				{
					vec3 sampleVS = pos_from_depth(fcoo*inv_d_wh, depth);
					vec3 sampleDirVS = sampleVS.xyz - P;
					float sampleLength = length(sampleDirVS);
					float occ = (dot(N, sampleDirVS)/sampleLength);
					if (occ>ANGLE_BIAS)
					{
						occ = (occ - ANGLE_BIAS)/(1.0-ANGLE_BIAS);
						float att = max(0.0,1.0-sampleLength / radius);
						occ_d = max(occ_d, occ * att);
					}
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

		float compute_shadow_map(vec3 P, vec3 N)
		{
			vec3 L = normalize(light_position - P);
			float dnl = max(0.0, dot(N, L));

			float bias_shd = bias_k.x + 0.0000000001* bias_k.y*tan(acos(dnl)); 
			vec4 sh_coord = shadow_matrix*vec4(P,1);
			float sc = 3.0/textureSize(TUshadow,0).x;
			vec3 shc =	vec3(sh_coord.xy, sh_coord.z  - bias_shd);
			float shad = texture(TUshadow, shc);
			for (int i=1;i<nb_samples;i++)
			{
				int index = int(15.99*random(gl_FragCoord.xyz,i));
				vec3 shc =	vec3(sh_coord.xy + texelFetch(TUpoisson,ivec2(index,0),0).xy*sc, sh_coord.z - bias_shd);
				shad += texture(TUshadow, shc);
			}
			return shad/float(nb_samples);
		}

	void main()
	{
		float depth = texture(TUdepth,tc).r;
		if (depth>=1.0)
		{
			vec2 stc = 2.0*tc -1.0;
			vec4 A = invMat*vec4(stc,0.0,1.0);
			vec4 B = invMat*vec4(stc,0.9,1.0);
			A/=A.w;
			B/=B.w;
			vec3 N = normalize(plane_n);
			//if (dot(P.xy,P.xy) < 150.0*150.0)
			if (N.z>0.0)
			{
				float z =  dot(N, plane_p - A.xyz) / dot(N,normalize(B.xyz-A.xyz));
				vec3 P = vec3(-stc/projv, 1.0) * z;
				frag_out = 8.0 + abs(P.x/200.0);//compute_shadow_map(P,N);
				return;
			}
			else
				discard;
		}

		vec3 P = pos_from_depth(tc,depth);
		vec3 N = N_from_ZB(tc,P);
		float hbao = max(0.0, 1.0 - compute_hbao(P,N) * ao_strength);
		float shadow = compute_shadow_map(P,N);
		frag_out = hbao*(shadow);
	}
	)";

	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("TUdepth", "projv", "fn", "radius", "subs", "nb_dirs", "nb_steps", "time", "ao_strength",
				 "light_position", "shadow_matrix", "TUshadow", "bias_k", "TUpoisson", "nb_samples", "invMat",
				 "plane_p", "plane_n");
}


void ShaderParamFullScreenHBAO::set_uniforms()
{
	shader_->set_uniforms_values(tex_d_->bind(0), projv_, fn_, radius_, subs_, nb_dirs_, nb_steps_, time_, ao_strength_,
								 light_position_, shadataptr_->shadow_matrix_,
								 shadataptr_->fbo_shadows_->getDepthTexture()->bind(1), shadataptr_->bias_k_,
		shadataptr_->tex_poisson_->bind(2), shadataptr_->nb_samples_, inv_mat_, plane_p_, plane_n_);
}


void ShaderParamFullScreenHBAO::draw(::cgogn::ui::View* v)
{
	const GLMat4d& mproj = v->projection_matrix_d();
	float64 znear = mproj(2, 3) / (mproj(2, 2) - 1.0);
	float64 zfar = mproj(2, 3) / (mproj(2, 2) + 1.0);

	projv_ = ::cgogn::rendering::GLVec2(mproj(0, 0), mproj(1, 1));
	fn_ = ::cgogn::rendering::GLVec3(float32(- 2.0 * zfar * znear), float32(zfar + znear), float32(zfar - znear));
	
	bind();
	glDrawArrays(GL_TRIANGLES, 0, 3);
	release();
}



ShaderFullScreenApplyHBAO* ShaderFullScreenApplyHBAO::instance_ = nullptr;

ShaderFullScreenApplyHBAO::ShaderFullScreenApplyHBAO()
{
	const char* vertex_shader_source = R"(
		#version 330
		out vec2 tc;
		void main()
		{
			tc = 2.0* vec2(gl_VertexID % 2, gl_VertexID / 2);
			gl_Position = vec4(2.0 * tc - 1.0, 0.0, 1.0);
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		in vec2 tc;
		out vec3 frag_out;

		uniform sampler2D TUambiant;
		uniform sampler2D TUdiffuse;
//		uniform float shadow_strength;

	void main()
	{
		vec3 diff = texture(TUdiffuse,tc).rgb;
		float ao_shadow = texture(TUambiant,tc).r;
		if (ao_shadow>=8.0)
			frag_out = vec3(ao_shadow-8.0);
		else;
		frag_out = diff * ao_shadow;
	}
	)";

	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("TUambiant", "TUdiffuse");//"shadow_strength");
}

void ShaderParamFullScreenApplyHBAO::set_uniforms()
{
	shader_->set_uniforms_values(tex_ambiant_->bind(0), tex_diffuse_->bind(1)); // , ??);
}

void ShaderParamFullScreenApplyHBAO::draw()
{
	bind();
	glDrawArrays(GL_TRIANGLES, 0, 3);
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
			tc = 2.0* vec2(gl_VertexID % 2, gl_VertexID / 2);
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
	glDrawArrays(GL_TRIANGLES, 0, 3);
	release();
}

void ShaderParamFSBlurAO::blurV()
{
	dtx_ = {0, 1}; 
	bind();
	glDrawArrays(GL_TRIANGLES, 0, 3);
	release();
}





} // namespace rendering

} // namespace cgogn
