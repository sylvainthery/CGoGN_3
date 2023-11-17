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

#ifndef CGOGN_RENDERING_SHADOWS_DATA_H_
#define CGOGN_RENDERING_SHADOWS_DATA_H_

#include <memory>
#include <cgogn/geometry/types/vector_traits.h>
#include <cgogn/rendering/fbo.h>
#include <cgogn/rendering/types.h>
#include <cgogn/rendering/shader_program.h>



namespace cgogn
{ 

namespace rendering
{

struct ShadowData
{
	static std::unique_ptr<cgogn::rendering::Texture2D> tex_poisson_;

	std::shared_ptr<cgogn::rendering::FBO> fbo_shadows_;
	GLMat4d shadow_matrix_;
	int nb_samples_;
	float32 bias_k_;

	ShadowData();

	inline bool is_started() const
	{
		return fbo_shadows_ != nullptr;
	}

	void start(double bias_k);

	inline void stop()
	{
		fbo_shadows_ = nullptr;
	}
};

//struct ShadowPlane
//{
//	static std::unique_ptr<cgogn::rendering::Texture2D> tex_plane_;
//	std::unique_ptr < cgogn::rendering::ShaderParamPlaneShadow> param_plane_;
//
//	ShadowPlane();
//	void init(cgogn::rendering::ShadowData* sha_dat_ptr);
//	void draw();
//};


std::string insert_shadow_code(const std::string& frag_src, const std::string& shadow_comment);

#define SHADOWS_UNIFORMS_STRINGS "with_shadow","shadow_matrix","TUshadow","bias_k","TUpoisson","nb_samples"

#define SHADOWS_PARAMETERS(ptr) ptr->shadow_matrix_,\
ptr->fbo_shadows_->getDepthTexture()->bind(14),\
ptr->bias_k_,\
ptr->tex_poisson_->bind(15),\
ptr->nb_samples_


template <typename T1, typename... Ts>
void sha_get_uniforms(cgogn::rendering::ShaderProgram* prg, T1 p1, Ts... pn)
{
	prg->get_uniforms(p1, pn..., SHADOWS_UNIFORMS_STRINGS);
}

template <typename T, typename... Ts>
 inline void sha_set_uniforms_values(cgogn::rendering::ShaderProgram* prg, ShadowData* sha_ptr, T v, Ts... vs)
{
	if (sha_ptr != nullptr)
		prg->set_uniforms_values(v, vs..., true, SHADOWS_PARAMETERS(sha_ptr));
	else
		prg->set_uniforms_values(v, vs..., false);
}

} // namespace rendering

} // namespace cgogn

#endif
