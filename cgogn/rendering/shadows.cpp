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


#include <cgogn/rendering/shadows.h>
#include <cgogn/rendering/types.h>

namespace cgogn
{

namespace rendering
{

Texture2D* ShadowData::tex_poisson_ = nullptr;

ShadowData::ShadowData() : fbo_shadows_(nullptr), nb_samples_(4), bias_k_(1e-27f)
{
	std::vector<float> pois = {-0.94201624f, -0.39906216f, 0.94558609f,	 -0.76890725f, -0.094184101, -0.92938870f,
							   0.34495938f,	 0.29387760f,  -0.91588581f, 0.45771432f,  -0.81544232f, -0.87912464f,
							   -0.38277543,	 0.27676845f,  0.97484398f,	 0.75648379f,  0.44323325f,	 -0.97511554f,
							   0.53742981f,	 -0.47373420f, -0.26496911,	 -0.41893023,  0.79197514,	 0.19090188f,
							   -0.24188840f, 0.99706507f,  -0.81409955,	 0.91437590f,  0.19984126f,	 0.78641367f,
							   0.14383161f,	 -0.14100790f};
	if (tex_poisson_ == nullptr)
	{
		tex_poisson_ = new Texture2D();
		tex_poisson_->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		tex_poisson_->allocate(16, 1, GL_RG32F, GL_RG, reinterpret_cast<uint8*>(pois.data()), GL_FLOAT);
	}
}

void ShadowData::start()
{
	fbo_shadows_ = std::make_shared<cgogn::rendering::FBO>(std::vector<std::shared_ptr<rendering::Texture2D>>{},
														   true, nullptr);
	fbo_shadows_->getDepthTexture()->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	fbo_shadows_->getDepthTexture()->release();
	int max_tex_sz;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_sz);
	int sz = std::min(max_tex_sz, 4096);
	fbo_shadows_->resize(sz, sz);
	started_ = true;
}




std::string insert_shadow_code(const std::string& frag_src, const std::string& shadow_comment)
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

	std::string frag_src_with_shadows{frag_src};
	auto i = frag_src_with_shadows.find(shadow_comment) + shadow_comment.length() + 1;
	frag_src_with_shadows.insert(i, src_shadows);
	return frag_src_with_shadows;
}

} // namespace rendering

} // namespace cgogn


