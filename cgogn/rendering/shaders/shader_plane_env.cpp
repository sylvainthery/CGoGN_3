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

#include <cgogn/rendering/shaders/shader_plane_env.h>

namespace cgogn
{

namespace rendering
{

ShaderPlaneShadow* ShaderPlaneShadow::instance_ = nullptr;

ShaderPlaneShadow::ShaderPlaneShadow()
{
	const char* vertex_shader_source = R"(
		#version 330
		uniform mat4 projection_matrix;
		uniform mat4 model_view_matrix;
		uniform mat4 shadow_matrix1;
		uniform mat4 shadow_matrix2;
		uniform mat4 transfo;
		uniform float scale_xy;

		out vec4 ShCoord1;
		out vec4 ShCoord2;
		out vec3 position;
		out vec2 tc;

		void main()
		{
			vec2 p = vec2(gl_VertexID % 2, gl_VertexID / 2);
			vec4 pt = transfo*vec4((2.0 * p - 1.0),0.0,1.0);
			tc = p*scale_xy;
			ShCoord1 = shadow_matrix1 * pt;
			ShCoord2 = shadow_matrix2 * pt;
			pt =  model_view_matrix*pt;
			position = pt.xyz;
			gl_Position = projection_matrix * pt; 
			// to avoid Z clipping (we do not use depth buffer heree
			gl_Position.z = -1.0 - pt.z; 
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		uniform vec4 color;
		uniform sampler2DShadow TUshadow1;
		uniform sampler2DShadow TUshadow2;
		uniform vec3 light_dir;
		uniform sampler2D TUcolor;
		
		in vec3 position;
		in vec4 ShCoord1;
		in vec4 ShCoord2;
		in vec2 tc;

		out vec4 frag_out;

		void main()
		{
			vec3 N = normalize(cross(dFdx(position), dFdy(position)));
			vec3 L = normalize(light_dir);
			float dnl = max(0.0, dot(N, L));
			vec3 shc1 = vec3(ShCoord1.xy/ShCoord1.w, clamp(ShCoord1.z/ShCoord1.w,0.0,1.0)); 
			shc1.z = clamp(shc1.z,0.0,1.0);
			vec3 shc2 = vec3(ShCoord2.xy/ShCoord2.w, clamp(ShCoord2.z/ShCoord2.w,0.0,1.0)); 
			shc2.z = clamp(shc2.z,0.0,1.0);

			float shadow =	(gl_FragCoord.z > 0.125) ? texture(TUshadow1, shc1) : texture(TUshadow2, shc2) * dnl;
			
			float lambert = 0.1 + 0.2*max(0.0,N.z) + 0.6 * shadow;
			frag_out = vec4(vec3(lambert * texture(TUcolor,tc).r), 1.0);
		}
	)";


	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("transfo", "scale_xy", "shadow_matrix1", "shadow_matrix2", "TUcolor", "TUshadow1", "TUshadow2",
				 "light_dir");
	nb_attributes_ = 0;
}

void ShaderParamPlaneShadow::set_uniforms()
{
	shader_->set_uniforms_values(transfo_, scale_xy_, sha_data_->shadow_matrix1_,
								 sha_data_->shadow_matrix2_, tex_col_->bind(0),
										   sha_data_->fbo_shadows1_->getDepthTexture()->bind(1),
											sha_data_->fbo_shadows2_->getDepthTexture()->bind(2),
								 sha_data_->light_dir_);
}

} // namespace rendering

} // namespace cgogn
