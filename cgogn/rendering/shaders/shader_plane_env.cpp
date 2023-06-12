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
		uniform mat4 shadow_matrix;
		uniform mat4 transfo;

		out vec3 ShCoord;
		out vec3 position;

		void main()
		{
			vec2 p = vec2(gl_VertexID % 2, gl_VertexID / 2);
			//tc = p;
			vec4 pt = transfo*vec4((2.0 * p - 1.0),0.0,1.0);
			vec4 shc4 = shadow_matrix * pt;
			ShCoord = shc4.xyz/shc4.w;
			pt =  model_view_matrix*pt;
			position = pt.xyz;
			gl_Position = projection_matrix * pt; 
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		uniform vec4 color;;
		uniform sampler2DShadow TUshadow;
		uniform vec3 light_position;
		
		in vec3 position;
		in vec3 ShCoord;
		out vec4 frag_out;

		float compute_shadow(float dnl)
		{
			return dnl*texture(TUshadow, ShCoord);
		}
		
		void main()
		{
			vec3 N = normalize(cross(dFdx(position), dFdy(position)));
			vec3 L = normalize(light_position-position);
			float dnl = max(0.0, dot(N, L));
			float shadow = dnl*texture(TUshadow, ShCoord);
			float lambert = 0.1 + 0.2*max(0.0,N.z) + 0.6 * shadow;

			frag_out = vec4(lambert * color.rgb, color.a);
		}
	)";


	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("transfo", "shadow_matrix", "color", "TUshadow", "light_position");
	nb_attributes_ = 0;
}

void ShaderParamPlaneShadow::set_uniforms()
{
	shader_->set_uniforms_values(transfo_,sha_data_->shadow_matrix_, color_,
										   sha_data_->fbo_shadows_->getDepthTexture()->bind(0),
								 sha_data_->light_position_);
}

} // namespace rendering

} // namespace cgogn
