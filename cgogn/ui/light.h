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

#ifndef CGOGN_UI_LIGHT_H_
#define CGOGN_UI_LIGHT_H_

#include <cgogn/ui/cgogn_ui_export.h>
#include <cgogn/ui/camera.h>


#include <cgogn/core/utils/numerics.h>
#include <cgogn/rendering/types.h>


using cgogn::rendering::GLVec3;
using cgogn::rendering::GLVec3d;
using cgogn::rendering::GLMat4d;


namespace cgogn
{

namespace ui
{

class CGOGN_UI_EXPORT LightData
{
public:
	// for interface imgui
	GLVec3 world_polar_;
	GLVec3 eye_polar_;
	bool light_on_cam_;

protected:
	const Camera* cam_;
	GLVec3 world_coord_;
	GLVec3 eye_coord_;

	inline void polar_to_cartesian(const GLVec3& polar,GLVec3& cartesian)
	{
		cartesian.z() = polar.z() * std::sin(polar.y());
		float r = polar.z() * std::cos(polar.y());
		cartesian.x() = r * std::cos(polar.x());
		cartesian.y() = r * std::sin(polar.x());
	}

	//inline void cartesian_to_polar(const GLVec3& cartesian, GLVec3& polar)
	//{
	//	polar.z() = cartesian.norm();
	//	GLVec3 car = cartesian.normalized();
	//	polar.y() = std::asin(car.z());
	//	polar.x() = float(std::acos(car.x() / std::cos(polar.y())));
	//	if (car.y() < 0.0)
	//		polar.x() *= -1.0f;
	//}



public:
	inline LightData():  world_polar_(-0.2f, 1.2f, 10), eye_polar_(1.4f, 0.5f, 10), light_on_cam_(false), cam_(nullptr),
		  world_coord_(0, 0, 0), eye_coord_(0, 0, 0)
	{
	}

	inline void link(const Camera& cptr)
	{
		cam_ = &cptr;
		update();
	}
	
	inline void update()
	{
		auto scene_center = cam_->pivot_point().cast<float>();

		if (light_on_cam_)
		{
			eye_polar_.z() = float(10.0 * cam_->scene_radius());
			polar_to_cartesian(eye_polar_, eye_coord_);
		}
		else
		{
			world_polar_.z() = float(10 * cam_->scene_radius());
			polar_to_cartesian(world_polar_, world_coord_);
			world_coord_ += scene_center;
		}
	}

	inline const GLVec3& getWorldCoord() const
	{
		if (light_on_cam_)
			return cgogn::rendering::homoTransform(cam_->mv_.inverse(),eye_coord_);
		else
			return  world_coord_;
	}
		
	inline GLVec3 getEyeCoord() const
	{
		if (light_on_cam_)
			return eye_coord_;
		else
			return cgogn::rendering::homoTransform(cam_->mv_, world_coord_);
	}

	//inline void setWorldCam(bool v)
	//{
	//	light_on_cam_ = v;
	//}
};





} // namespace ui

} // namespace cgogn

#endif // CGOGN_UI_LIGHT_H_
