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

#include <cgogn/rendering/shaders/shader_full_screen_shadow.h>
#include<cgogn/ui/view.h>


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
			gl_Position = vec4(2.0 * tc - 1.0, 0.0, 1.0);
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		in vec2 tc;
		out vec3 frag_out;

		uniform sampler2D TUdepth;
		uniform vec2 projv;  // [ projection_matrix[0][0],projection_matrix.data[1][1] ];
		uniform vec3 fn;    // [ -2.0*zfar*znearn, zfar+znear, zfar-znear ];
		uniform float radius;
		uniform float subs;
		uniform int nb_dirs;
		uniform int nb_steps;
		uniform float time;

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

		vec3 min_len_vect(vec3 P, vec3 Pr, vec3 Pl)
		{
		  vec3 V1 = Pr - P;
		  vec3 V2 = P - Pl;
		  return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
		}

		vec3 Ntc_from_ZB(in float d, in vec3 P)
		{
			vec2 ts =  vec2(1.5)/textureSize(TUdepth,0);
			vec3 Pr = pos_from_depth(tc+vec2(ts.x,0.0),d);
			vec3 Pl = pos_from_depth(tc+vec2(-ts.x,0.0),d);
			vec3 Pb = pos_from_depth(tc+vec2(0.0,ts.y),d);
			vec3 Pt = pos_from_depth(tc+vec2(0.0,-ts.y),d);
			return normalize(cross(min_len_vect(P, Pr, Pl), min_len_vect(P, Pb, Pt)));
		}

		
	void main()
	{
		float depth = texture(TUdepth,tc).r;
		if (depth>=1.0)
			discard;
		vec3 P = pos_from_depth(tc,depth);
		vec3 Np = Ntc_from_ZB(depth, P);

		float projectedRad = - radius/2.0 * projv.x / P.z;
		float screenRadius = projectedRad * textureSize(TUdepth,0).x;
		if (screenRadius < 3.0)
		{
			frag_out = vec3(0.0);
			return;
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
					float occ = (dot(Np, sampleDirVS)/sampleLength);
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
		frag_out = vec3(1.0 - occlusion / float(nb_dirs)) *0.00001 +abs(Np);// vec3(abs(P.xy),0.0);//  
	}
	)";

	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("TUdepth", "projv", "fn", "radius", "subs", "nb_dirs", "nb_steps", "time");
}

void ShaderParamFullScreenHBAO::set_uniforms()
{
	shader_->set_uniforms_values(tex_d_->bind(1), projv_, fn_, radius_, subs_, nb_dirs_, nb_steps_, time_);
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

} // namespace rendering

} // namespace cgogn
