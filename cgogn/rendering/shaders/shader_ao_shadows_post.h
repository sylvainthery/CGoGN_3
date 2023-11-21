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
 * for more details.            

 */
#ifndef CGOGN_RENDERING_SHADERS_FULL_SCREEN_SHADOW_H_
#define CGOGN_RENDERING_SHADERS_FULL_SCREEN_SHADOW_H_

#include <cgogn/rendering/cgogn_rendering_export.h>
#include <cgogn/rendering/shader_program.h>
#include <cgogn/rendering/texture.h>

namespace cgogn
{
namespace ui
{
class View;
}
}

using ::cgogn::rendering::GLVec2;
using ::cgogn::rendering::GLVec3;
using ::cgogn::rendering::GLVec4;
using ::cgogn::rendering::GLMat4;


namespace cgogn
{

namespace rendering
{
//forward
struct ShadowData;

DECLARE_SHADER_CLASS(FullScreenHBAO, false, CGOGN_STR(FullScreenHBAO))

class CGOGN_RENDERING_EXPORT ShaderParamFullScreenHBAO : public ShaderParam
{
	void set_uniforms() override;

public:
	std::shared_ptr<::cgogn::rendering::Texture2D> tex_d_;
	std::shared_ptr<::cgogn::rendering::Texture2D> tex_n_;
	std::shared_ptr<::cgogn::rendering::Texture2D> tex_shadow_;
	std::shared_ptr<::cgogn::rendering::Texture2D> tex_poisson_;
	GLVec2 projv_;
	GLVec3 fn_;
	float radius_;
	float subs_;
	int nb_dirs_;
	int nb_steps_;
	float time_;
	float ao_strength_;
	ShadowData* shadataptr_;
	GLVec3 light_position_;
	
	GLMat4 shadow_matrix_;
	float bias_k_;
	int nb_samples_;
	GLMat4 inv_mat_;
	GLVec3 plane_p_;
	GLVec3 plane_n_;
	
	using ShaderType = ShaderFullScreenHBAO;

	inline ShaderParamFullScreenHBAO(ShaderType* sh)
		: ShaderParam(sh), tex_d_(nullptr), radius_(0.0f), subs_(1.0f), nb_dirs_(7), nb_steps_(7), time_(1.0f),
		  ao_strength_(1.0f),light_position_(10, 100,1000)
		 {}

	inline ~ShaderParamFullScreenHBAO() override
	{
	}
	
	void ShaderParamFullScreenHBAO::draw(::cgogn::ui::View* v);
};


DECLARE_SHADER_CLASS(FullScreenApplyHBAO, false, CGOGN_STR(FullScreenApplyHBAO))

class CGOGN_RENDERING_EXPORT ShaderParamFullScreenApplyHBAO : public ShaderParam
{
	void set_uniforms() override;

public:
	float ambiant_ratio_;
	float hb_ka;
	std::shared_ptr<::cgogn::rendering::Texture2D> tex_ambiant_;
	std::shared_ptr<::cgogn::rendering::Texture2D> tex_diffuse_;
	//std::shared_ptr<::cgogn::rendering::Texture2D> tex_spec_; // TODO

	using ShaderType = ShaderFullScreenApplyHBAO;

	inline ShaderParamFullScreenApplyHBAO(ShaderType* sh)
		: ShaderParam(sh), ambiant_ratio_(0.5), tex_ambiant_(nullptr), tex_diffuse_(nullptr)
	{
	}

	inline ~ShaderParamFullScreenApplyHBAO() override
	{
	}

	void ShaderParamFullScreenApplyHBAO::draw();
};



DECLARE_SHADER_CLASS(FSBlurAO, false, CGOGN_STR(FSBlurAO))

class CGOGN_RENDERING_EXPORT ShaderParamFSBlurAO : public ShaderParam
{
	void set_uniforms() override;

public:
	std::array<int32, 2> dtx_;
	std::shared_ptr<::cgogn::rendering::Texture2D> tex_;

	using ShaderType = ShaderFSBlurAO;

	inline ShaderParamFSBlurAO(ShaderType* sh)
		: ShaderParam(sh), dtx_{1,0}, tex_(nullptr)
	{
	}

	inline ~ShaderParamFSBlurAO() override
	{
	}

	void ShaderParamFSBlurAO::blurH();
	void ShaderParamFSBlurAO::blurV();
};

} // namespace rendering

} // namespace cgogn

#endif
