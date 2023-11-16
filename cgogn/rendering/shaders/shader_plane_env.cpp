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
		uniform mat4 transfo;
		uniform float scale_xy;

		out vec3 position;
		out vec2 tc;

		void main()
		{
			vec2 p = vec2(gl_VertexID % 2, gl_VertexID / 2); 
			tc = p*scale_xy;
			vec4 pt =  model_view_matrix*transfo*vec4((2.0 * p - 1.0), 0.0, 1.0);
			position = pt.xyz;
			vec4 q = projection_matrix * pt; 
			gl_Position = vec4(q.x, q.y, 0.5*q.w, q.w);
			// to avoid Z clipping (we do not use depth buffer here)
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		uniform sampler2D TUcolor;
		uniform vec3 light_position;
	
		in vec3 position;
		in vec2 tc;

		out vec4 frag_out;

		uniform bool with_shadow;
		uniform sampler2DShadow TUshadow;
		uniform sampler2D TUpoisson;
		uniform mat4 shadow_matrix;
		uniform int nb_samples;

		float random(vec3 seed)
		{
			float dot_product = dot(seed, vec3(12.9898,78.233,45.164));
			return fract(sin(dot_product) * 43758.5453);
		}

		float compute_shadow(float dnl)
		{
			if (!with_shadow)
				return 1.0;

			vec4 sh_coord = shadow_matrix*vec4(position,1); 
			float sc = 7.0/textureSize(TUshadow,0).x;	
			float shad = texture(TUshadow, vec3(sh_coord.xy/sh_coord.w, sh_coord.z /sh_coord.w));
			for (int i=1;i<nb_samples;i++)
			{
				int index = int(15.99*random(gl_FragCoord.xyz));
				vec3 shc =	vec3(sh_coord.xy/sh_coord.w + texelFetch(TUpoisson,ivec2(index,0),0).xy*sc, sh_coord.z /sh_coord.w);
				shad += texture(TUshadow, shc);
			}
			return shad/float(nb_samples);
		}

		void main()
		{
			vec3 N = normalize(cross(dFdx(position), dFdy(position)));
			vec3 L = normalize(light_position - position);
			float dnl = max(0.0, dot(N, L));
			float lambert = 0.2 + 0.1*max(0.0,N.z) + 0.7 * dnl * compute_shadow( dnl);
			frag_out = vec4(vec3(lambert * texture(TUcolor,tc).r), 1.0);
		}

	)";


	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("transfo", "scale_xy", "TUcolor", "light_position", "with_shadow",
				 "shadow_matrix", "TUshadow", "TUpoisson", "nb_samples");
}

void ShaderParamPlaneShadow::set_uniforms()
{
	if (sha_data_ != nullptr)
		shader_->set_uniforms_values(transfo_, scale_xy_, tex_col_->bind(0), light_position_,
								 true, sha_data_->shadow_matrix_, sha_data_->fbo_shadows_->getDepthTexture()->bind(1),
									 sha_data_->tex_poisson_->bind(2), sha_data_->nb_samples_);
	else
		shader_->set_uniforms_values(transfo_, scale_xy_, tex_col_->bind(0), light_position_, false);

}

} // namespace rendering

} // namespace cgogn
