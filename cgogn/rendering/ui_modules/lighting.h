 /*******************************************************************************
 * CGoGN                                                                        *
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

#ifndef CGOGN_MODULE_LIGHTING_H_
#define CGOGN_MODULE_LIGHTING_H_

#include <cgogn/core/ui_modules/mesh_provider.h>
#include <cgogn/ui/app.h>
#include <cgogn/ui/imgui_helpers.h>
#include <cgogn/ui/module.h>
#include <cgogn/ui/view.h>
#include <cgogn/rendering/shaders/shader_plane_env.h>

#include <cgogn/rendering/shadows.h>

#include <cgogn/geometry/types/vector_traits.h>

#include <GLFW/glfw3.h>
#include <boost/synapse/connect.hpp>
#include <memory>
#include <unordered_map>

using cgogn::rendering::GLVec3d;
using cgogn::rendering::GLVec4d;
using cgogn::rendering::GLMat4d;

namespace cgogn
{

namespace ui
{

using cgogn::rendering::GLVec3d;
using cgogn::rendering::GLVec4d;
using cgogn::rendering::GLMat4d;


struct LightData
{
	GLVec3d light_world_coord_;
	GLVec3d light_eye_coord_;

	bool light_on_cam_;
	inline LightData() : light_world_coord_(0, 0, 0), light_eye_coord_(0, 0, 0), light_on_cam_(true)
	{
	}

	void setLightWorld(const GLVec3d& P)
	{
		light_world_coord_ = P;
	}

	inline void update(const GLMat4d& modelview_mat, const GLMat4d& inv_modelview_mat)
	{
		if (light_on_cam_)
			light_world_coord_ = cgogn::rendering::homoTransform(inv_modelview_mat, light_eye_coord_);
		else
			light_eye_coord_ = cgogn::rendering::homoTransform(modelview_mat, light_world_coord_);
	}
};


class LightingModule : public ViewModule
{

	struct ViewParameters
	{
		LightData light_;
		bool use_shadow_;
		cgogn::rendering::ShadowData shadow_;
		std::unique_ptr<cgogn::rendering::ShaderPlaneShadow::Param> param_plane_;
		bool use_plane_;

		ViewParameters()
			: use_plane_(false)
		{
			param_plane_->sha_data_ = &shadow_;
			param_plane_->tex_col_ = std::make_shared<rendering::Texture2D>();

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
			param_plane_->tex_col_->allocate(tex_sz, tex_sz, GL_R8, GL_RED, tex_data.data(), GL_UNSIGNED_BYTE);
			param_plane_->tex_col_->bind();
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			param_plane_->tex_col_->release();
		}
	};


public:
	LightingModule(const App& app)
		: ViewModule(app, "LightingModule")
	{
	}

	~LightingModule()
	{
	}


public:

	inline void start_shadows()
	{
		view_parameters_[app_.current_view()].shadow_.start();
	}

	inline void stop_shadows()
	{
		view_parameters_[app_.current_view()].shadow_.stop();
	}


	void create_shadowMap()
	{
		auto& vp = view_parameters_[app_.current_view()];
		const Camera& cam = app_.current_view()->camera();
		float64 sr = cam.scene_radius();
		LightData& light = vp.light_;

		if (vp.shadow_.is_started())
		{
			auto orthographic = [&](float64 zcenter) {
				float64 znear = zcenter - sr;
				float64 zfar = zcenter + 5.0 * sr;
				float64 ihw = 1.0 / sr;
				float64 r_inv = 1.0 / (znear - zfar);
				GLMat4d m;
				m << ihw, 0, 0, 0, 0, ihw, 0, 0, 0, 0, 2.0 * r_inv, (znear + zfar) * r_inv, 0, 0, 0, 1;
				return m;
			};

			auto look_at = [](const GLVec3d& eye, const GLVec3d& at,
							  const GLVec3d& up) {
				GLVec3d zAxis = (eye - at).normalized();
				GLVec3d xAxis = up.cross(zAxis).normalized();
				GLVec3d yAxis = zAxis.cross(xAxis).normalized();

				GLMat4d trf;
				trf.block<1, 3>(0, 0) = xAxis.transpose();
				trf.block<1, 3>(1, 0) = yAxis.transpose();
				trf.block<1, 3>(2, 0) = zAxis.transpose();
				trf.block<3, 1>(0, 3) = GLVec3d(-xAxis.dot(eye), -yAxis.dot(eye), -zAxis.dot(eye));
				trf.block<1, 4>(3, 0) = rendering::GLVec4d(0, 0, 0, 1).transpose();
				return trf;
			};

			GLMat4d biasmat;
			biasmat << 0.5, 0.0, 0.0, 0.5, 0.0, 0.5, 0.0, 0.5, 0.0, 0.0, 0.5, 0.5, 0.0, 0.0, 0.0, 1.0;

			const GLVec3d& center = cam.pivot_point();
			GLMat4d light_projection_matrix = orthographic((light.light_world_coord_ - center).norm());
			GLMat4d light_view_matrix = look_at(light.light_world_coord_, center, GLVec3d(0, 1, 0));

			vp.shadow_.fbo_shadows_->bind();

			glEnable(GL_DEPTH_TEST);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			selected_view_->draw_shadowmap(light_projection_matrix, light_view_matrix);
			vp.shadow_.fbo_shadows_->release();
		}
	}


	void init() override
	{
//		start_timer= std::chrono::high_resolution_clock::now();
	}

	void draw(View* view) override
	{
	}

	void left_panel() override
	{
		bool need_update = false;

//		if (app_.nb_views() > 1)
//			imgui_view_selector(this, selected_view_, [&](View* v) { selected_view_ = v; });

//		if (selected_view_)
//		{
//			ViewParameters& vp = view_parameters_[selected_view_];
//			if (ImGui::Checkbox("Shadows", &vp.use_shadows_))
//			{
//				set_shadows(selected_view_, vp.use_shadows_);
//				need_update = true;
//			}
//			if (vp.use_shadows_)
//			{
//				if (ImGui::SliderFloat("Bias", &vp.vm_bias, 15,27))
//				{
//					vp.shadow_.bias_k_ = float(diag) / std::pow(2.0f, vp.vm_bias);
//					need_update = true;
//				}
//				if (ImGui::SliderInt("samples", &vp.sha_data_.nb_samples_, 1, 9))
//					need_update = true;
//			}
//			if (ImGui::Checkbox("Light on cam", &vp.light_on_cam_))
//				need_update = true;

//			if (ImGui::SliderFloat("Light_X", &vp.light_., -10.0f, 10.0f))
//				need_update = true;
//			if (ImGui::SliderFloat("Light_Y", &vp.lightPosY_, -10.0f, 10.0f))
//				need_update = true;
//			if (ImGui::SliderFloat("Light_Z", &vp.lightPosZ_, 0.0f, 20.0f))
//				need_update = true;

		//			ImGui::Separator();
		//			if (ImGui::SliderFloat("PlaneZ", &vp.Zplaneshift_, 0.0f, 2.0f))
		//				need_update = true;
	}

private:
	std::chrono::high_resolution_clock::time_point start_timer;
	float poff1 = -1.0;
	float poff2 = -1.0;
	View* selected_view_;
	std::unordered_map<View*, ViewParameters> view_parameters_;
//	std::vector<std::shared_ptr<boost::synapse::connection>> connections_;

};

} // namespace ui

} // namespace cgogn

#endif // CGOGN_MODULE_SURFACE_RENDER_H_
