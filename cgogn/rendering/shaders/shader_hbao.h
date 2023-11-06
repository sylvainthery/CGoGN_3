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

#ifndef CGOGN_RENDERING_SHADERS_HBAO_H_
#define CGOGN_RENDERING_SHADERS_HBAO_H_

#include <cgogn/rendering/cgogn_rendering_export.h>
#include <cgogn/rendering/shader_program.h>
#include <cgogn/rendering/texture.h>

namespace cgogn
{

namespace rendering
{

DECLARE_SHADER_CLASS(HBAO, false, CGOGN_STR(HBAO))

class CGOGN_RENDERING_EXPORT ShaderParamHBAO : public ShaderParam
{
	void set_uniforms() override;

public:
	GLMat4d projection_matrix_;
	std::shared_ptr<Texture2D> depth_texture_;
	GLint unit_;
	float width_;
	float height_;
	float radius_;
	float subs_;
	int nb_dirs_;
	int nb_steps_;
	GLVec3 far_near_precomp_values_;
	GLVec2 proj_values_;

	using ShaderType = ShaderHBAO;

	ShaderParamHBAO(ShaderType* sh) : ShaderParam(sh), unit_(0), alpha_(1.0f)
	{
	}

	inline ~ShaderParamHNBAO() override
	{
	}

	inline void draw()
	{
		bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		release();
	}
};

} // namespace rendering

} // namespace cgogn

#endif // CGOGN_RENDERING_SHADERS_FULL_SCREEN_TEXTURE_H_
