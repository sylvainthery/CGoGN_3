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
#include <cgogn/rendering/shaders/shader_explode_volumes_def.h>

namespace cgogn
{

namespace rendering
{

ShaderExplodeVolumes1* ShaderExplodeVolumes1::instance_ = nullptr;

ShaderExplodeVolumes1::ShaderExplodeVolumes1()
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
		out vec3 frag_out;

		void main()
		{
			vec3 N = normalize(cross(dFdx(position), dFdy(position)));
			frag_out = N;
		}
	)";

	load(vertex_shader_source,fragment_shader_source);
	get_uniforms("vertex_ind", "vertex_position", "volume_center", "volume_clipping", "explode", "plane_clip", "plane_clip2");
	nb_attributes_ = 2; // ???
}

void ShaderParamExplodeVolumes1::set_uniforms()
{
	shader_->set_uniforms_values(10, 11, 12, 13, data_->explode_,
							data_->plane_clip_, data_->plane_clip2_);
}

void ShaderParamExplodeVolumes1::bind_texture_buffers()
{
	vbos_[VERTEX_POSITION]->bind_texture_buffer(11);
	vbos_[VOLUME_CENTER]->bind_texture_buffer(12);
	vbos_[VOLUME_CLIPPING]->bind_texture_buffer(13);
}

void ShaderParamExplodeVolumes1::release_texture_buffers()
{
	vbos_[VERTEX_POSITION]->release_texture_buffer(11);
	vbos_[VOLUME_CENTER]->release_texture_buffer(12);
	vbos_[VOLUME_CLIPPING]->release_texture_buffer(13);
}


ShaderExplodeVolumes1smooth* ShaderExplodeVolumes1smooth::instance_ = nullptr;

ShaderExplodeVolumes1smooth::ShaderExplodeVolumes1smooth()
{
	const char* vertex_shader_source = R"(
		#version 330
		uniform mat4 projection_matrix;
		uniform mat4 model_view_matrix;
		uniform mat3 normal_matrix;	

		uniform usamplerBuffer vertex_ind;
		uniform samplerBuffer vertex_position;
		uniform samplerBuffer volume_center;
		uniform samplerBuffer volume_clipping;

		uniform float explode;
		uniform vec4 plane_clip;
		uniform vec4 plane_clip2;

		out vec3 normal;


		void main()
		{
			int ind_c = int(texelFetch(vertex_ind, 10 * gl_InstanceID + 9).r);

			vec3 center = texelFetch(volume_center, ind_c).rgb;
			vec3 clip_center = texelFetch(volume_clipping, ind_c).rgb;

			float d = dot(plane_clip, vec4(clip_center, 1.0));
			float d2 = dot(plane_clip2, vec4(clip_center, 1.0));
			if (d <= 0.0 && d2 <= 0.0)
			{
				int iii =  10 * gl_InstanceID + 3*gl_VertexID;
				int ind_v = int(texelFetch(vertex_ind,iii).r);
				vec3 position_in = texelFetch(vertex_position, ind_v).rgb;
				vec3 explode_position = mix(center, position_in, explode);
				vec4 position4 = model_view_matrix * vec4(explode_position, 1);
				gl_Position = projection_matrix * position4;

				vec3 Pprev = texelFetch(vertex_position, int(texelFetch(vertex_ind, ++iii).r)).xyz;
				vec3 Pnext = texelFetch(vertex_position, int(texelFetch(vertex_ind, ++iii).r)).xyz;
				normal = normal_matrix * normalize(cross(Pnext-position_in.xyz,Pprev-position_in.xyz));
			}
			else
				gl_Position = vec4(0.0, 0.0, 0.0, 1.0); // check
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330

		in vec3 normal;
		out vec3 frag_out;

		void main()
		{
			frag_out = normalize(normal);
		}
	)";

	load(vertex_shader_source, fragment_shader_source);

	get_uniforms("vertex_ind", "vertex_position", "volume_center", "volume_clipping", "explode", "plane_clip", "plane_clip2");

	nb_attributes_ = 2;
}

void ShaderParamExplodeVolumes1smooth::set_uniforms()
{
	shader_->set_uniforms_values(10, 11, 12, 13, data_->explode_, data_->plane_clip_, data_->plane_clip2_);
}

void ShaderParamExplodeVolumes1smooth::bind_texture_buffers()
{
	vbos_[VERTEX_POSITION]->bind_texture_buffer(11);
	vbos_[VOLUME_CENTER]->bind_texture_buffer(12);
	vbos_[VOLUME_CLIPPING]->bind_texture_buffer(13);
}

void ShaderParamExplodeVolumes1smooth::release_texture_buffers()
{
	vbos_[VERTEX_POSITION]->release_texture_buffer(11);
	vbos_[VOLUME_CENTER]->release_texture_buffer(12);
	vbos_[VOLUME_CLIPPING]->release_texture_buffer(13);
}



ShaderExplodeVolumes2* ShaderExplodeVolumes2::instance_ = nullptr;


ShaderExplodeVolumes2::ShaderExplodeVolumes2()
{
	const char* vertex_shader_source = R"(
		#version 330
		out vec2 tc;
		void main()
		{
			vec2 p = 2.0 * vec2(gl_VertexID % 2, gl_VertexID / 2);
			tc = p;
			gl_Position = vec4(2.0 * p - 1.0, 0.0, 1.0);
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330
		in vec2 tc;
		out vec4 frag_out;

		uniform sampler2D TUd;
		uniform sampler2D TUn;
		uniform vec2 projv;// [ projection_matrix[0][0],projection_matrix.data[1][1] ];
		uniform vec3 fn; // [ -2.0*zfar*znearn, zfar+znear, zfar-znear ];
		uniform vec4 color;
		uniform vec3 light_position;

//Shadows_code_here


		vec3 XYZfromDepth(vec2 uv, float d)
		{
			float z_n = 2.0 * d - 1.0;
			float z = fn.x / (fn.y - z_n * (fn.z));
			return vec3((-2.0*uv-1.0)/projv*z, z);
		}
		
		void main()
		{
			float depth = texture(TUd,tc).r;

			if (depth>=1.0)
				discard;

			vec3 position = XYZfromDepth(tc,depth);
			vec3 L = normalize(light_position - position);
			vec3 N = normalize(texture(TUn,tc).rgb);
			float dnl = max(0.0, dot(N, L));
			float lambert = 0.2 + + 0.8 * dnl * compute_shadow(position, dnl);
			frag_out = vec4(lambert * color.rgb, color.a);
		}
	)";

	load(vertex_shader_source, insert_shadow_code(fragment_shader_source, "//Shadows_code_here"));
	sha_get_uniforms(this, "TUd", "TUn", "projv", "fn", "color", "light_position");
}

void ShaderParamExplodeVolumes2::set_uniforms()
{
	sha_set_uniforms_values(shader_,sha_data_, tex_d_->bind(0), tex_n_->bind(1),projv_,fn_,
		data_->color_, data_->light_position_);
}


} // namespace rendering

} // namespace cgogn
