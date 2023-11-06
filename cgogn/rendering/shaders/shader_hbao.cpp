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

#include <cgogn/rendering/shaders/shader_hbao.h>
#include <iostream>

namespace cgogn
{

namespace rendering
{

ShaderHBAO* ShaderHBAO::instance_ = nullptr;

ShaderHBAO::ShaderHBAO()
{
const char* vertex_shader_source = R"(
#version 330
out vec2 tc;
void main()
{
	uint vid = uint(gl_VertexID);
	tc = vec2(vid%2u, vid/2u);
	gl_Position = vec4(2.0*tc-1.0, 0.0, 1.0);
})";

const char* fragment_shader_source = R"(
#version 300 es
precision highp float;
in vec2 tc;
out float frag_out;

uniform highp sampler2D TUdepth;
uniform mat4 projectionMatrix;
uniform float width;
uniform float height;
uniform float radius;
uniform float subs;
uniform int nb_dirs;
uniform int nb_steps;
uniform vec3 far_near_precomp_values; // (-2*f*n, f+n, f-n)
#define TWO_PI  6.28318531
#define PI      3.141592654

// sin(PI/?)
#define ANGLE_BIAS 0.13

float rand1Dp(vec2 coord) //generating noise/pattern texture for dithering
{
	return fract(sin(dot(coord ,vec2(12.9898,78.233))) * 43758.5453);
}

float ZfromDepth(float d)
{
	// precompute an uniform vec3 with (-2*f*n, f+n, f-n) ?
	float z_n = 2.0 * d - 1.0;
	return far_near_precomp_values.x / (far_near_precomp_values.y - z_n * (far_near_precomp_values.z));
}

vec3 XYZfromDepth(vec2 uv, float d)
{
	float z = ZfromDepth(d);
	vec2 uvs =	2.0*uv-1.0;
	return vec3(-uvs/vec2(projectionMatrix[0][0], projectionMatrix[1][1])*z, z);
}

vec3 XYZfromDepth(vec2 uv)
{
	float d = texture(TUdepth,uv).x;
	return XYZfromDepth(uv,d);
}

vec3 minLenDiff(vec3 P, vec3 Pr, vec3 Pl)
{
	vec3 V1 = Pr - P;
	vec3 V2 = P - Pl;
	return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 reconstructNormal(vec2 uv, vec3 P)
{
	float ex = 1.0/float(width*subs);
	float ey = 1.0/float(height*subs);
	vec3 Pr = XYZfromDepth(uv+vec2(ex,0.0));
	vec3 Pl = XYZfromDepth(uv+vec2(-ex,0.0));
	vec3 Pb = XYZfromDepth(uv+vec2(0.0,ey));
	vec3 Pt = XYZfromDepth(uv+vec2(0.0,-ey));
	return normalize(cross(minLenDiff(P, Pr, Pl), minLenDiff(P, Pb, Pt)));
}


void main()
{
	float depth = texture(TUdepth,tc).r;
	if (depth>=1.0)
		discard;
	vec3 P = XYZfromDepth(tc,depth);

	float projectedRad = - radius/2.0 * projectionMatrix[0][0] / P.z;
	float screenRadius = projectedRad * width;

	if (screenRadius < 3.0)
	{
		frag_out = 0.0;
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
	vec3 Nb = reconstructNormal(tc,P);

	vec2 inv_d_wh = vec2(1)/vec2(width,height);

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
				vec3 sampleVS = XYZfromDepth(fcoo*inv_d_wh, depth);
				vec3 sampleDirVS = sampleVS.xyz - P;
				float sampleLength = length(sampleDirVS);
				float occ = (dot(Nb, sampleDirVS)/sampleLength);
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
	frag_out = occlusion / float(nb_dirs);
}
)";

	load2_bind(vertex_shader_source, fragment_shader_source);
	get_uniforms("TUdepth","projectionMatrix","width","height","radius","subs","nb_dirs","nb_steps","far_near_precomp_values");
}

void ShaderParamHBAO::set_uniforms()
{
	double znear = projection_matrix_(3,2)/(projection_matrix_(2,2)-1.0);
	double zfar = ((projection_matrix_(2,2)-1.0)*znear)/(projection_matrix_(2,2)+1.0);
	GLVec3 far_near_precomp_values{float(-2.0 * zfar * znear), float(zfar+znear), float(zfar - znear)};
	GLVec3 proj_values{projection_matrix_(2,2), projection_matrix_(2,2)};
	shader_->set_uniforms_values(depth_texture_->bind(unit_),width_,height_,radius_,subs_,nb_dirs_,nb_steps_,far_near_precomp_values,proj_values)
} // namespace rendering

} // namespace cgogn
