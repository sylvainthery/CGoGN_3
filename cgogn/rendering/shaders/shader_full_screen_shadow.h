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

DECLARE_SHADER_CLASS(FullScreenHBAO, true, CGOGN_STR(FullScreenHBAO))

class CGOGN_RENDERING_EXPORT ShaderParamFullScreenHBAO : public ShaderParam
{
	void set_uniforms() override;

public:
	std::shared_ptr<::cgogn::rendering::Texture2D> tex_d_;
	GLVec2 projv_;
	GLVec3 fn_;
	float radius_;
	float subs_;
	int nb_dirs_;
	int nb_steps_;
	float time_;



	using ShaderType = ShaderFullScreenHBAO;

	inline ShaderParamFullScreenHBAO(ShaderType* sh)
		: ShaderParam(sh), tex_d_(nullptr), radius_(0.0f), subs_(1.0f), nb_dirs_(7), nb_steps_(7), time_(1.0f)
	{
	}

	inline ~ShaderParamFullScreenHBAO() override
	{
	}
	
	void ShaderParamFullScreenHBAO::draw(::cgogn::ui::View* v);
};


} // namespace rendering

} // namespace cgogn

#endif
