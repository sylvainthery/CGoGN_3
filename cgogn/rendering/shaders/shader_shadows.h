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

#ifndef CGOGN_RENDERING_SHADERS_SHADOWS_H_
#define CGOGN_RENDERING_SHADERS_SHADOWS_H_

namespace cgogn
{

namespace rendering
{

const char* src_shadows = R"(
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
			float sc = 2.0/textureSize(TUshadow,0).x;
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
)";

#define SHADOWS_UNIFORMS_STRINGS "with_shadow","shadow_matrix","TUshadow","bias_k","TUpoisson","nb_samples"
#define SHADOWS_PARAMETERS sha_data_->shadow_matrix_,\
sha_data_->fbo_shadows_->getDepthTexture()->bind(0),\
sha_data_->bias_k_,\
sha_data_->tex_poisson_.bind(1),\
sha_data_->nb_samples_


} // namespace rendering

} // namespace cgogn

#endif
