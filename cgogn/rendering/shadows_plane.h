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

#ifndef CGOGN_RENDERING_SHADOWS_PLANE_H_
#define CGOGN_RENDERING_SHADOWS_PLANE_H_

//#include <memory>
#include <cgogn/rendering/types.h>
#include <cgogn/rendering/fbo.h>
#include <cgogn/rendering/shaders/shader_plane_env.h>


using cgogn::rendering::GLVec3;
using cgogn::rendering::GLMat4;


namespace cgogn
{

namespace rendering
{

struct ShadowsPlane
{
	static std::shared_ptr<cgogn::rendering::Texture2D> tex_plane_;
	std::unique_ptr<cgogn::rendering::ShaderParamPlaneShadow> param_plane_;

	ShadowsPlane();
	void init(cgogn::rendering::ShadowData* sha_dat_ptr);
	void drawZ(const std::pair<GLVec3d, GLVec3d>& bb, float64 shift, const GLMat4d& projm, const GLMat4d& mvm,
			   const GLVec3& light_position);
};


} // namespace rendering

} // namespace cgogn

#endif
