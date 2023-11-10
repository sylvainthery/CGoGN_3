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

#include <cgogn/rendering/fbo.h>

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

using geometry::Vec3;
using geometry::Scalar;


struct ShadowData
{
	bool started_;
	GLMat4d shadow_matrix_;
	std::shared_ptr<cgogn::rendering::FBO> fbo_shadows_;
	cgogn::rendering::Texture2D tex_poisson_;
	int nb_samples_;
	float32 bias_k_;

	inline ShadowData()
		: fbo_shadows_(nullptr), tex_poisson_({{GL_TEXTURE_MIN_FILTER, GL_NEAREST},
											   {GL_TEXTURE_MAG_FILTER, GL_NEAREST},
											   {GL_TEXTURE_WRAP_S, GL_REPEAT},
											   {GL_TEXTURE_WRAP_T, GL_REPEAT}}),
		  nb_samples_(4), bias_k_(1e-27f)
	{
		std::vector<float> pois = {-0.94201624f, -0.39906216f, 0.94558609f,	 -0.76890725f, -0.094184101, -0.92938870f,
								   0.34495938f,	 0.29387760f,  -0.91588581f, 0.45771432f,  -0.81544232f, -0.87912464f,
								   -0.38277543,	 0.27676845f,  0.97484398f,	 0.75648379f,  0.44323325f,	 -0.97511554f,
								   0.53742981f,	 -0.47373420f, -0.26496911,	 -0.41893023,  0.79197514,	 0.19090188f,
								   -0.24188840f, 0.99706507f,  -0.81409955,	 0.91437590f,  0.19984126f,	 0.78641367f,
								   0.14383161f,	 -0.14100790f};
		tex_poisson_.allocate(16, 1, GL_RG32F, GL_RG, reinterpret_cast<uint8*>(pois.data()), GL_FLOAT);
	}

	inline bool is_started() const
	{
		return fbo_shadows_ != nullptr;
	}

	inline void start()
	{
		fbo_shadows_ =
			std::make_shared<cgogn::rendering::FBO>(std::vector<std::shared_ptr<rendering::Texture2D>>{}, true, nullptr);
		fbo_shadows_->getDepthTexture()->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		fbo_shadows_->getDepthTexture()->release();
		int max_tex_sz;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_sz);
		int sz = std::min(max_tex_sz, 4096);
		fbo_shadows_->resize(sz, sz);
		started_ = true;
	}

	inline void stop()
	{
		fbo_shadows_ = nullptr;
	}
};


struct LightData
{
	GLVec3d light_world_coord_;
	GLVec3d light_eye_coord_;

	bool light_on_cam_;
	inline LightData()
		: light_world_coord_(0, 0, 0), light_eye_coord_(0, 0, 0), use_light_(true), use_shadow_(false),
		  light_on_cam_(true)
	{
	}

	void setLightWorld(const GLVec3d& P)
	{
		light_world_coord_ = P;
	}

	inline void update(const rendering::GLMat4d& modelview_mat, const rendering::GLMat4d& inv_modelview_mat)
	{
		if (light_on_cam_)
			light_world_coord_ = rendering::homoTransform(inv_modelview_mat, light_eye_coord_);
		else
			light_eye_coord_ = rendering::homoTransform(modelview_mat, light_world_coord_);
	}
};


class LightingModule : public ViewModule
{

	struct ViewParameters
	{
		LightData light_;
		bool use_shadow_;
		ShadowData shadow_;
		std::unique_ptr<rendering::ShaderPlaneShadow::Param> param_plane_;
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

protected:
	std::unordered_map<View*, ViewParameters> view_parameters_;

//	std::vector<std::shared_ptr<boost::synapse::connection>> connections_;


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
		view_parameters_[app.current_view()].shadow_.start();
	}

	inline void stop_shadows()
	{
		view_parameters_[app.current_view()].shadow_.stop();
	}


	void create_shadowMap()
	{
		auto& vp = view_parameters_[app.current_view()];
		const Camera& cam = app.current_view()->camera();
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
			selected_view_->draw_shadowmap(light_projection_matrix.cast<float>(), light_view_matrix.cast<float>());
			vp.sha_data_.fbo_shadows_->release();
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

		if (selected_view_)
		{
			ViewParameters& vp = view_parameters_[selected_view_];
			if (ImGui::Checkbox("Shadows", &vp.use_shadows_))
			{
				set_shadows(selected_view_, vp.use_shadows_);
				need_update = true;
			}
			if (vp.use_shadows_)
			{
				if (ImGui::SliderFloat("Bias", &vp.vm_bias, 15,27))
				{
					vp.shadow_.bias_k_ = float(diag) / std::pow(2.0f, vp.vm_bias);
					need_update = true;
				}
				if (ImGui::SliderInt("samples", &vp.sha_data_.nb_samples_, 1, 9))
					need_update = true;
			}
			if (ImGui::Checkbox("Light on cam", &vp.light_on_cam_))
				need_update = true;

			if (ImGui::SliderFloat("Light_X", &vp.light_., -10.0f, 10.0f))
				need_update = true;
			if (ImGui::SliderFloat("Light_Y", &vp.lightPosY_, -10.0f, 10.0f))
				need_update = true;
			if (ImGui::SliderFloat("Light_Z", &vp.lightPosZ_, 0.0f, 20.0f))
				need_update = true;

			ImGui::Separator();
			if (ImGui::SliderFloat("PlaneZ", &vp.Zplaneshift_, 0.0f, 2.0f))
				need_update = true;

			if (selected_view_ && selected_mesh_)
			{
				Parameters& p = parameters_[selected_view_][selected_mesh_];	

				imgui_combo_attribute<Vertex, Vec3>(*selected_mesh_, p.vertex_position_, "Position",
													[&](const std::shared_ptr<Attribute<Vec3>>& attribute) {
														set_vertex_position(*selected_view_, *selected_mesh_,
																			attribute);
													});

				ImGui::Separator();
				need_update |= ImGui::Checkbox("Vertices", &p.render_vertices_);
				if (p.render_vertices_)
				{
					need_update |= ImGui::ColorEdit3("Color##vertices", p.param_point_sprite_->color_.data(),
													 ImGuiColorEditFlags_NoInputs);
					need_update |= ImGui::SliderFloat("Size##vertices", &p.vertex_scale_factor_, 0.1f, 2.0f);
				}

				ImGui::Separator();
				need_update |= ImGui::Checkbox("Edges", &p.render_edges_);
				if (p.render_edges_)
				{
					need_update |= ImGui::ColorEdit3("Color##edges", p.param_bold_line_->color_.data(),
													 ImGuiColorEditFlags_NoInputs);
					need_update |= ImGui::SliderFloat("Width##edges", &p.param_bold_line_->width_, 1.0f, 10.0f);
				}

				ImGui::Separator();
				need_update |= ImGui::Checkbox("Volumes", &p.render_volumes_);
				if (p.render_volumes_)
				{
					need_update |= ImGui::Checkbox("Cast Shadow", &p.cast_shadow_);

					if (ImGui::Checkbox("Smooth faces", &p.smooth_volume_faces_))
						need_update = true;

					if (ImGui::Checkbox("shadowable", &p.receive_shadows_))
						need_update = true;

					if (ImGui::SliderFloat("Explode", &p.data_.explode_, 0.01f, 1.0f))
						need_update = true;
					
					auto bb = mesh_provider_->meshes_bb();

					//View* view = app_.current_view();
					//ViewParameters& vp = view_parameters_[view];

					need_update |= ImGui::Checkbox("Volume lines", &p.render_volume_lines_);
					if (p.render_volume_lines_)
						need_update |= ImGui::ColorEdit3("Volume lines color", p.data_.color_line_.data(),
														 ImGuiColorEditFlags_NoInputs);

					need_update |= ImGui::Checkbox("Apply clipping plane", &p.clipping_plane_);
					if (p.clipping_plane_)
					{
						imgui_combo_attribute<Vertex, Vec3>(
							*selected_mesh_, p.vertex_clipping_position_, "Clipping Position",
							[&](const std::shared_ptr<Attribute<Vec3>>& attribute) {
								set_vertex_clipping_position(*selected_view_, *selected_mesh_, attribute);
							});

						need_update |= ImGui::Checkbox("Clip only volumes", &p.clip_only_volumes_);

						Vec3 position;
						p.frame_manipulator_.get_position(position);
						Vec3 axis_z;
						p.frame_manipulator_.get_axis(rendering::FrameManipulator::Zt, axis_z);
						float32 d = -(position.dot(axis_z));
						rendering::GLVec4 plane = rendering::construct_GLVec4(axis_z.x(), axis_z.y(), axis_z.z(), d);

						if (p.clip_only_volumes_)
						{
							p.param_point_sprite_->plane_clip_ = {0, 0, 0, 0};
							p.param_bold_line_->plane_clip_ = {0, 0, 0, 0};
						}
						else
						{
							p.param_point_sprite_->plane_clip_ = plane;
							p.param_bold_line_->plane_clip_ = plane;
						}
						p.data_.plane_clip_ = plane;
					}
					else
					{
						p.param_point_sprite_->plane_clip_ = {0, 0, 0, 0};
						p.param_bold_line_->plane_clip_ = {0, 0, 0, 0};
						p.data_.plane_clip_ = {0, 0, 0, 0};
					}

					need_update |= ImGui::Checkbox("Show clipping plane", &p.show_frame_manipulator_);
					if (p.show_frame_manipulator_)
						ImGui::TextUnformatted("Press C to manipulate the plane");

					ImGui::TextUnformatted("Colors");
					ImGui::BeginGroup();
					if (ImGui::RadioButton("Global##color", p.color_per_cell_ == GLOBAL))
					{
						p.color_per_cell_ = GLOBAL;
						need_update = true;
					}
					ImGui::SameLine();
					if (ImGui::RadioButton("Per volume##color", p.color_per_cell_ == PER_VOLUME))
					{
						p.color_per_cell_ = PER_VOLUME;
						need_update = true;
					}

					if (p.color_per_cell_ == GLOBAL)
					{
						need_update |= ImGui::ColorEdit3("Volume color", p.param_volume_->data_->color_.data(),
														 ImGuiColorEditFlags_NoInputs);
					}
					else if (p.color_per_cell_ == PER_VOLUME)
					{
						ImGui::BeginGroup();
						if (ImGui::RadioButton("Scalar", p.color_type_ == SCALAR))
						{
							p.color_type_ = SCALAR;
							need_update = true;
						}
						ImGui::SameLine();
						if (ImGui::RadioButton("Vector", p.color_type_ == VECTOR))
						{
							p.color_type_ = VECTOR;
							need_update = true;
						}
						ImGui::EndGroup();

						if (p.color_type_ == SCALAR)
						{
							imgui_combo_attribute<Volume, Scalar>(
								*selected_mesh_, p.volume_scalar_, "Attribute##scalarvolumecolor",
								[&](const std::shared_ptr<Attribute<Scalar>>& attribute) {
									set_volume_scalar(*selected_view_, *selected_mesh_, attribute);
								});
							need_update |= ImGui::InputFloat("Scalar min##volumecolor", &p.data_.color_map_.min_value_,
															 0.01f, 1.0f, "%.3f");
							need_update |= ImGui::InputFloat("Scalar max##volumecolor", &p.data_.color_map_.max_value_,
															 0.01f, 1.0f, "%.3f");
							if (ImGui::Checkbox("Auto update min/max##volumecolor",
												&p.auto_update_volume_scalar_min_max_))
							{
								if (p.auto_update_volume_scalar_min_max_)
								{
									update_volume_scalar_min_max_values(p);
									need_update = true;
								}
							}
						}
						else if (p.color_type_ == VECTOR)
						{
							imgui_combo_attribute<Volume, Vec3>(
								*selected_mesh_, p.volume_color_, "Attribute##vectorvolumecolor",
								[&](const std::shared_ptr<Attribute<Vec3>>& attribute) {
									set_volume_color(*selected_view_, *selected_mesh_, attribute);
								});
						}
					}
					ImGui::EndGroup();
					need_update |= ImGui::SliderFloat("POFF1", &poff1, -3.0f, 3.0f, "%.3f");
					need_update |= ImGui::SliderFloat("POFF2", &poff2, -3.0f, 3.0f, "%.3f");
				}

				float64 remain = mesh_provider_->mesh_data(*selected_mesh_).outlined_until_ - App::frame_time_;
				if (remain > 0)
					need_update = true;

				if (need_update)
					for (View* v : linked_views_)
						v->request_update();

				// ViewParameters& vp = view_parameters_[selected_view_];
				// if (vp.use_shadows_)
				//	selected_view_->request_update();
			}
		}
	}

private:
	std::chrono::high_resolution_clock::time_point start_timer;
	float poff1 = -1.0;
	float poff2 = -1.0;
	View* selected_view_;
	const MESH* selected_mesh_;
	std::unordered_map<View*, std::unordered_map<const MESH*, Parameters>> parameters_;
	std::unordered_map<View*, ViewParameters> view_parameters_;
	std::vector<std::shared_ptr<boost::synapse::connection>> connections_;
	std::unordered_map<const MESH*, std::vector<std::shared_ptr<boost::synapse::connection>>> mesh_connections_;
	MeshProvider<MESH>* mesh_provider_;

	rendering::Outliner* outline_engine_;
	// std::unique_ptr<rendering::ComputeVolumeCenterEngine> compute_volume_center_engine_;
};

} // namespace ui

} // namespace cgogn

#endif // CGOGN_MODULE_SURFACE_RENDER_H_
