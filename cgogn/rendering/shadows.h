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

#ifndef CGOGN_RENDERING_SHADOWS_H_
#define CGOGN_RENDERING_SHADOWS_H_

#include <memory>
#include <cgogn/geometry/types/vector_traits.h>
#include <cgogn/rendering/fbo.h>
#include <cgogn/rendering/types.h>


namespace cgogn
{

namespace rendering
{

struct ShadowData
{
	bool started_;
	GLMat4d shadow_matrix_;
	std::shared_ptr<cgogn::rendering::FBO> fbo_shadows_;

	static cgogn::rendering::Texture2D* tex_poisson_;

	int nb_samples_;
	float32 bias_k_;

	ShadowData();

	inline bool is_started() const
	{
		return fbo_shadows_ != nullptr;
	}

	void start();

	inline void stop()
	{
		fbo_shadows_ = nullptr;
	}
};



std::string insert_shadow_code(const std::string& frag_src, const std::string& shadow_comment);

#define SHADOWS_UNIFORMS_STRINGS "with_shadow","shadow_matrix","TUshadow","bias_k","TUpoisson","nb_samples"

#define SHADOWS_PARAMETERS(ptr) ptr->shadow_matrix_,\
ptr->fbo_shadows_->getDepthTexture()->bind(0),\
ptr->bias_k_,\
ptr->tex_poisson_->bind(1),\
ptr->nb_samples_


} // namespace rendering

} // namespace cgogn

#endif
