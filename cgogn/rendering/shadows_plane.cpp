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


#include <cgogn/rendering/shadows_plane.h>
#include <cgogn/rendering/types.h>

using cgogn::rendering::GLVec2;
using cgogn::rendering::GLVec3;
using cgogn::rendering::GLVec4;
using cgogn::rendering::GLMat4;

using cgogn::rendering::GLVec2d;
using cgogn::rendering::GLVec3d;
using cgogn::rendering::GLVec4d;
using cgogn::rendering::GLVec4d;

namespace cgogn
{

namespace rendering
{

std::shared_ptr<Texture2D> ShadowsPlane::tex_plane_ = nullptr;

ShadowsPlane::ShadowsPlane()
{
	if (tex_plane_ == nullptr)
	{
		tex_plane_ = std::make_shared<cgogn::rendering::Texture2D>();
		std::vector<uint8> tex_data;
		const uint32 tex_sz = 256;
		tex_data.reserve(tex_sz * tex_sz);
		for (int i = 0; i < tex_sz; ++i)
			tex_data.push_back(0);
		for (int j = 1; j < tex_sz; ++j)
		{
			tex_data.push_back(0);
			for (int i = 1; i < tex_sz; ++i)
				tex_data.push_back(240);
		}
		tex_plane_->allocate(tex_sz, tex_sz, GL_R8, GL_RED, tex_data.data(), GL_UNSIGNED_BYTE);
		tex_plane_->bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		tex_plane_->release();
	}

	param_plane_ = cgogn::rendering::ShaderPlaneShadow::generate_param();
	param_plane_->tex_col_ = tex_plane_;
	
}

void ShadowsPlane::init(ShadowData* ptr)
{
	param_plane_->sha_data_ = ptr;
}

void ShadowsPlane::drawZ(const std::pair<GLVec3d, GLVec3d>& bb, float64 shift, const GLMat4& projm, const GLMat4& mvm, const GLVec3& light_position)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	auto bb_sz = (bb.second - bb.first);
	auto bb_center = (bb.second + bb.first) / 2;
	
	Eigen::Transform<float64, 3, Eigen::Affine> trf =
		Eigen::Translation3d(Eigen::Vector3d(bb_center.x(), bb_center.y(),
											 bb.first.z() - shift * bb_sz.z())) *
		Eigen::Scaling((1+shift)*bb_sz);
	
	param_plane_->transfo_ = trf.matrix().cast<float>();
	param_plane_->scale_xy_ = 20.0f;
	param_plane_->light_position_ = light_position;
	param_plane_->draw(projm, mvm);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

} // namespace rendering

} // namespace cgogn


