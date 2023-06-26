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

#ifndef CGOGN_RENDERING_SHADERS_EXPLODE_VOLUMES_DATA_H_
#define CGOGN_RENDERING_SHADERS_EXPLODE_VOLUMES_DATA_H_

#include <cgogn/rendering/cgogn_rendering_export.h>
#include <cgogn/rendering/types.h>
#include <cgogn/rendering/fbo.h>
#include <cgogn/rendering/shaders/shader_function_color_maps.h>

namespace cgogn
{

namespace rendering
{

//data of shader (can be shared between flat/smooth version)
struct ExplodeVolumeData
{
	GLVec4 plane_clip_;
	GLVec4 plane_clip2_;
	GLColor color_;
	GLColor color_line_;
	shader_function::ColorMap::Uniforms color_map_;
	GLMat4 shadow_matrix_;
	GLVec3 light_dir_;
	std::shared_ptr<FBO> fbo_shadows_;
	GLVec3 light_position_;
	float32 explode_;

	inline ExplodeVolumeData()
		: plane_clip_(0, 0, 0, 0), plane_clip2_(0, 0, 0, 0), color_(0.9f, 0, 0, 1),
		  light_dir_(0,0,0), fbo_shadows_(nullptr), explode_(0.9f)
	{}
};

struct ShadowData
{
	GLMat4d shadow_matrix_;
	GLVec3d light_position_;
	GLVec3d light_dir_;
	std::shared_ptr<FBO> fbo_shadows_;
	int32 use_shadows_;

	inline ShadowData() : fbo_shadows_(nullptr), use_shadows_(0)
	{
	}

	inline void start()
	{
		fbo_shadows_ =
			std::make_shared<rendering::FBO>(std::vector<std::shared_ptr<rendering::Texture2D>>{}, true, nullptr);
		fbo_shadows_->getDepthTexture()->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		fbo_shadows_->getDepthTexture()->release();
		int32 sz_tex = std::pow(2,use_shadows_);
		fbo_shadows_->resize(sz_tex,sz_tex); 
	}

	inline void stop()
	{
		fbo_shadows_ = nullptr;
	}
};


} // namespace rendering

} // namespace cgogn

#endif
