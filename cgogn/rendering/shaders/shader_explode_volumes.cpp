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

#include <cgogn/rendering/shaders/shader_explode_volumes.h>

namespace cgogn
{

namespace rendering
{

ShaderExplodeVolumes* ShaderExplodeVolumes::instance_ = nullptr;

ShaderExplodeVolumes::ShaderExplodeVolumes()
{
	const char* vertex_shader_source = R"(
		#version 330
		uniform mat4 projection_matrix;
		uniform mat4 model_view_matrix;

		uniform usamplerBuffer vertex_ind;
		uniform samplerBuffer vertex_position;
		uniform samplerBuffer volume_center;
		uniform samplerBuffer volume_clipping;

		uniform float explode;
		uniform vec4 plane_clip;
		uniform vec4 plane_clip2;

		uniform bool smooth_face;
		out vec3 position;

		void main()
		{
			int ind_c = int(texelFetch(vertex_ind, 4 * gl_InstanceID + 3).r);
			vec3 center = texelFetch(volume_center, ind_c).rgb;
			vec3 clip_center = texelFetch(volume_clipping, ind_c).rgb;

			float d = dot(plane_clip, vec4(clip_center, 1.0));
			float d2 = dot(plane_clip2, vec4(clip_center, 1.0));
			if (d <= 0.0 && d2 <= 0.0)
			{
				int ind_v = int(texelFetch(vertex_ind, 4 * gl_InstanceID + gl_VertexID).r);
				vec3 position_in = texelFetch(vertex_position, ind_v).rgb;
				vec3 explode_position = mix(center, position_in, explode);
				vec4 position4 = model_view_matrix * vec4(explode_position, 1);
				position = position4.xyz;
				gl_Position = projection_matrix * position4;
			}
			else
			{
				gl_Position = vec4(0.0, 0.0, 0.0, 1.0); // check
			}
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330

		in vec3 position;
		out vec4 frag_out;

		uniform vec4 color;
		uniform vec3 light_position;


		uniform bool with_shadow;
		uniform float bias_k;
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

			float bias_shd = bias_k+bias_k*tan(acos(dnl));
			vec4 sh_coord = shadow_matrix*vec4(position,1);
			float sc = 3.0/textureSize(TUshadow,0).x;
			vec3 shc =	vec3(sh_coord.xy/sh_coord.w, sh_coord.z /sh_coord.w - bias_shd);
			float shad = texture(TUshadow, shc);
			for (int i=1;i<nb_samples;i++)
			{
				int index = int(15.99*random(gl_FragCoord.xyz));
				vec3 shc =	vec3(sh_coord.xy/sh_coord.w + texelFetch(TUpoisson,ivec2(index,0),0).xy*sc, sh_coord.z /sh_coord.w - bias_shd);
				shad += texture(TUshadow, shc);
			}
			return shad/float(nb_samples);
		}

		void main()
		{
			vec3 N = normalize(cross(dFdx(position), dFdy(position)));
			vec3 L = normalize(light_position - position);

			float dnl = max(0.0, dot(N, L));
			float lambert = 0.15 + 0.05*max(0.0,N.z) + 0.8 * dnl * compute_shadow(dnl);
			frag_out = vec4(lambert * color.rgb, color.a);
		}
	)";

	load(vertex_shader_source, fragment_shader_source);
	get_uniforms("vertex_ind", "vertex_position", "volume_center", "volume_clipping", "color", "light_position",
				 "explode", "plane_clip", "plane_clip2", "with_shadow", "shadow_matrix", "TUshadow", "bias_k",
				 "TUpoisson", "nb_samples");
	nb_attributes_ = 2; // ???
}

void ShaderParamExplodeVolumes::set_uniforms()
{
	if (sha_data_ != nullptr)
		shader_->set_uniforms_values(10, 11, 12, 13, data_->color_, data_->light_position_,
									 data_->explode_, data_->plane_clip_, data_->plane_clip2_,
									 true, sha_data_->shadow_matrix_, sha_data_->fbo_shadows_->getDepthTexture()->bind(0), sha_data_->bias_k_,
									 sha_data_->tex_poisson_->bind(1), sha_data_->nb_samples_);
	else
		shader_->set_uniforms_values(10, 11, 12, 13, data_->color_, data_->light_position_, data_->explode_,
									 data_->plane_clip_, data_->plane_clip2_, false);
}

void ShaderParamExplodeVolumes::bind_texture_buffers()
{
	vbos_[VERTEX_POSITION]->bind_texture_buffer(11);
	vbos_[VOLUME_CENTER]->bind_texture_buffer(12);
	vbos_[VOLUME_CLIPPING]->bind_texture_buffer(13);
}

void ShaderParamExplodeVolumes::release_texture_buffers()
{
	vbos_[VERTEX_POSITION]->release_texture_buffer(11);
	vbos_[VOLUME_CENTER]->release_texture_buffer(12);
	vbos_[VOLUME_CLIPPING]->release_texture_buffer(13);
}



} // namespace rendering

} // namespace cgogn
