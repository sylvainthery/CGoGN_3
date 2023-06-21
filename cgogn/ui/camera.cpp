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

#include <cgogn/ui/camera.h>

namespace cgogn
{

namespace ui
{

rendering::GLMat4d Camera::perspective(float64 znear, float64 zfar) const
{
	float64 range_inv = 1.0 / (znear - zfar);
	float64 f = 1.0 / std::tan(field_of_view_ / 2.0);
	auto m05 = (aspect_ratio_ > 1) ? std::make_pair(f / aspect_ratio_, f) : std::make_pair(f, f * aspect_ratio_);
	rendering::GLMat4d m;
	m << m05.first, 0, 0, 0, 0, m05.second, 0, 0, 0, 0, (znear + zfar) * range_inv, 2 * znear * zfar * range_inv, 0, 0,
		-1, 0;
	return m;
}

rendering::GLMat4d Camera::orthographic(float64 znear, float64 zfar) const
{
	float64 range_inv = 1.0 / (znear - zfar);
	float64 hw = 1.414 / scene_radius_;
	auto m05 = std::make_pair(hw,hw);
//		(aspect_ratio_ < 1) ? std::make_pair(1.0 / aspect_ratio_, 1.0) : std::make_pair(1.0, 1.0 / aspect_ratio_);
	rendering::GLMat4d m;
	m << m05.first, 0, 0, 0, 0, m05.second, 0, 0, 0, 0, 2 * range_inv, 0, 0, 0, (znear + zfar) * range_inv, 1;	
	return m;
}

rendering::GLMat4d Camera::orthographic(double l, double r, double b, double t, double zfar, double znear) 
{
	float64 inv_w = 1.0 / (r - l);
	float64 inv_h = 1.0 / (t- b);
	float64 inv_z = 1.0 / (znear - zfar);
	rendering::GLMat4d m;
	m << 2.0 * inv_w, 0.0, 0.0, -(r + l) * inv_w,
		0.0, 2.0 * inv_h, 0.0, -(t + b) * inv_h, 
		0.0, 0.0, 0.0, 2.0 * inv_z, (znear + zfar) * inv_z,
		0.0, 0.0, 0.0, 1.0;
	return m;
}

} // namespace ui

} // namespace cgogn
