/*******************************************************************************
 * CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
 * Copyright (C, IGG Group, ICube, University of Strasbourg, France            *
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
	GLVec3 light_position_;
	float32 explode_;

	inline ExplodeVolumeData()
		: plane_clip_(0, 0, 0, 0), plane_clip2_(0, 0, 0, 0), color_(0.9f, 0, 0, 1), light_position_(1000, 1000, 5000),
		explode_(0.9f)
	{}
};

struct ShadowData
{
	GLMat4d shadow_matrix_;
	std::shared_ptr<FBO> fbo_shadows_;
	Texture2D tex_poisson_;
	int nb_samples_;
	float32 bias_k_;
	bool use_shadows_;

	inline ShadowData()
		: fbo_shadows_(nullptr), tex_poisson_({{GL_TEXTURE_MIN_FILTER, GL_NEAREST},
											   {GL_TEXTURE_MAG_FILTER, GL_NEAREST},
											   {GL_TEXTURE_WRAP_S, GL_REPEAT},
											   {GL_TEXTURE_WRAP_T, GL_REPEAT}}),
		  nb_samples_(4), bias_k_(0.0001f), use_shadows_(false)
	{
		std::vector<float> pois = {
			-0.94201624f, -0.39906216f, 0.94558609f,  -0.76890725f, -0.094184101, -0.92938870f, 0.34495938f, 0.29387760f,
			-0.91588581f, 0.45771432f,  -0.81544232f, -0.87912464f, -0.38277543,  0.27676845f,  0.97484398f, 0.75648379f,
			0.44323325f,	 -0.97511554f, 0.53742981f,  -0.47373420f, -0.26496911,  -0.41893023, 0.79197514, 0.19090188f,
			-0.24188840f, 0.99706507f,  -0.81409955, 0.91437590f,	0.19984126f,	  0.78641367f,  0.14383161f, -0.14100790f};
		tex_poisson_.allocate(16, 1, GL_RG32F, GL_RG, reinterpret_cast<uint8*>(pois.data()), GL_FLOAT);
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
		int max_tex_sz;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_sz);
		int sz = std::min(max_tex_sz, 4096);
		fbo_shadows_->resize(sz,sz); 

	}


	inline void stop()
	{
		fbo_shadows_ = nullptr;
	}
};

// A METTRE EN MODULE
struct LightData
{
	bool light_on_cam_;
	rendering::GLVec3d light_world_coord_;
	rendering::GLVec3d light_eye_coord_;

	inline void update(const rendering::GLMat4d& modelview_mat, const rendering::GLMat4d& inv_modelview_mat)
	{
		if (light_on_cam_)
			light_world_coord_ = rendering::homoTransform(inv_modelview_mat, light_eye_coord_);
		else
			light_eye_coord_ = rendering::homoTransform(modelview_mat, light_world_coord_);
	}
};



} // namespace rendering

} // namespace cgogn

#endif
