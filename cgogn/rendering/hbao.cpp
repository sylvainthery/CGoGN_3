
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

#include <cgogn/rendering/hbao.h>

namespace cgogn
{

namespace rendering
{

HBAO::HBAO()
{
}

void HBAO::init()
{
	auto tex_ao = std::make_shared<rendering::Texture2D>();
	tex_ao->allocate(1, 1, GL_R32F, GL_RED);
	fbo_ao_ = std::make_unique<rendering::FBO>({tex_ao.get()});

	auto tex_blur = std::make_shared<rendering::Texture2D>();
	tex_blur->allocate(1, 1, GL_R32F, GL_RED);
	fbo_blur_ = std::make_unique<rendering::FBO>({tex_blur.get()});

	prg_ssao,prg_blur_ao,

void HBAO::compute(const rendering::Texture2D* depth_tex, float rad, int steps, int rots, int subs, int blurs)
{

}



} // namespace rendering
} // namespace cgogn
