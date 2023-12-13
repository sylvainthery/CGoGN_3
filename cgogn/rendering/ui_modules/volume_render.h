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

#ifndef CGOGN_MODULE_VOLUME_RENDER_H_
#define CGOGN_MODULE_VOLUME_RENDER_H_

#include <cgogn/core/ui_modules/mesh_provider.h>
#include <cgogn/ui/app.h>
#include <cgogn/ui/imgui_helpers.h>
#include <cgogn/ui/module.h>
#include <cgogn/ui/view.h>

#include <cgogn/geometry/types/vector_traits.h>

#include <cgogn/rendering/frame_manipulator.h>
// #include <cgogn/rendering/shaders/compute_volume_centers.h>
#include <cgogn/rendering/shaders/outliner.h>
#include <cgogn/rendering/shaders/shader_bold_line.h>
#include <cgogn/rendering/shaders/shader_explode_volumes.h>
#include <cgogn/rendering/shaders/shader_explode_volumes_smooth.h>
#include <cgogn/rendering/shaders/shader_explode_volumes_color.h>
#include <cgogn/rendering/shaders/shader_explode_volumes_line.h>
#include <cgogn/rendering/shaders/shader_explode_volumes_scalar.h>
#include <cgogn/rendering/shaders/shader_explode_volumes_shadows.h>
#include <cgogn/rendering/fbo.h>
#include <cgogn/rendering/shaders/shader_point_sprite.h>
#include <cgogn/rendering/shaders/shader_plane_env.h>
#include <cgogn/rendering/shadows.h>

#include <cgogn/geometry/algos/centroid.h>
#include <cgogn/geometry/algos/length.h>

#include <GLFW/glfw3.h>
#include <boost/synapse/connect.hpp>

#include <unordered_map>

namespace cgogn
{

namespace ui
{

using geometry::Scalar;
using geometry::Vec3;

template <typename MESH>
class VolumeRender : public ViewModule
{
	static_assert(mesh_traits<MESH>::dimension >= 3, "VolumeRender can only be used with meshes of dimension >= 3");

	enum AttributePerCell
	{
		GLOBAL = 0,
		PER_VOLUME
	};
	enum ColorType
	{
		SCALAR = 0,
		VECTOR
	};

	template <typename T>
	using Attribute = typename mesh_traits<MESH>::template Attribute<T>;

	using Vertex = typename mesh_traits<MESH>::Vertex;
	using Edge = typename mesh_traits<MESH>::Edge;
	using Volume = typename mesh_traits<MESH>::Volume;

	using Vec3 = geometry::Vec3;
	using Scalar = geometry::Scalar;

	struct Parameters
	{
		Parameters()
			: vertex_position_(nullptr), vertex_position_vbo_(nullptr), vertex_clipping_position_(nullptr),
			  volume_clipping_position_(nullptr), volume_clipping_position_vbo_(nullptr), volume_scalar_(nullptr),
			  volume_scalar_vbo_(nullptr), volume_color_(nullptr), volume_color_vbo_(nullptr), volume_center_(nullptr),
			  volume_center_vbo_(nullptr), render_vertices_(false), render_edges_(false), render_volumes_(true),
			  render_volume_lines_(false), cast_shadow_(false), receive_shadows_(false), smooth_volume_faces_(false),
			  color_per_cell_(GLOBAL), color_type_(SCALAR), vertex_scale_factor_(1.0),
			  auto_update_volume_scalar_min_max_(true), clipping_plane_(false), clip_only_volumes_(true),
			  show_frame_manipulator_(false), manipulating_frame_(false)
		{
			transfo_ = rendering::Transfo3d::Identity();
			transfo_scene_ = rendering::Transfo3d::Identity();

			data_.color_ = {0.4f, 0.8f, 1.0f, 1.0f};
			data_.color_line_ = {0.0f, 0.0f, 0.0f, 1.0f};
		}

		template <typename TS>
		void register_shader_explvol()
		{
			auto s = TS::generate_param();
			s->data_ = &this->data_;
			s->sha_data_ = nullptr;
			params_volumes_.push_back(std::move(s));

			auto s2 = TS::generate_param();
			s2->data_ = &this->data_;
			s2->sha_data_ = this->sha_data_;
			params_volumes_.push_back(std::move(s2));
		}

		void init_shaders(rendering::ShadowData* sha_data_ptr)
		{
			param_point_sprite_ = rendering::ShaderPointSprite::generate_param();
			param_point_sprite_->color_ = rendering::GLColor(1, 0.5f, 0, 1);

			param_bold_line_ = rendering::ShaderBoldLine::generate_param();
			param_bold_line_->color_ = {1.0f, 1.0f, 1.0f, 1.0f};
			param_bold_line_->width_ = 2.0f;

			param_volume_line_ = rendering::ShaderExplodeVolumesLine::generate_param();
			param_volume_line_->data_ = &this->data_;

			param_volume_generate_shadows_ = rendering::ShaderExplodeVolumesGenerateShadows::generate_param();
			param_volume_generate_shadows_->data_ = &this->data_;

			sha_data_ = sha_data_ptr;
			register_shader_explvol<rendering::ShaderExplodeVolumes>();
			register_shader_explvol<rendering::ShaderExplodeVolumesSmooth>();
			register_shader_explvol<rendering::ShaderExplodeVolumesColor>();
			register_shader_explvol<rendering::ShaderExplodeVolumesColorSmooth>();
			register_shader_explvol<rendering::ShaderExplodeVolumesScalar>();
			register_shader_explvol<rendering::ShaderExplodeVolumesScalarSmooth>();
		}

		inline std::pair<GLVec3d, GLVec3d> compute_initial_transfo(const std::pair<GLVec3d, GLVec3d>& bb)
		{
			auto bb_width = bb.second - bb.first;
			auto center = bb.second + bb.first / 2;
			float64 width = std::max(bb_width.x(), std::max(bb_width.y(), bb_width.z()));
			transfo_ = Eigen::Translation3d(GLVec3d(-center)) * Eigen::Scaling(2.0 / width);
			return {transfo_ * bb.first, transfo_ * bb.second};
		}

		CGOGN_NOT_COPYABLE_NOR_MOVABLE(Parameters);

		std::shared_ptr<Attribute<Vec3>> vertex_position_;
		rendering::VBO* vertex_position_vbo_;

		std::shared_ptr<Attribute<Vec3>> vertex_clipping_position_;
		rendering::VBO* vertex_clipping_position_vbo_;

		std::shared_ptr<Attribute<Vec3>> volume_clipping_position_;
		rendering::VBO* volume_clipping_position_vbo_;

		std::shared_ptr<Attribute<Scalar>> volume_scalar_;
		rendering::VBO* volume_scalar_vbo_;
		std::shared_ptr<Attribute<Vec3>> volume_color_;
		rendering::VBO* volume_color_vbo_;

		std::shared_ptr<Attribute<Vec3>> volume_center_;
		rendering::VBO* volume_center_vbo_;

		rendering::ExplodeVolumeData data_;
		rendering::ShadowData* sha_data_;

		std::unique_ptr<rendering::ShaderPointSprite::Param> param_point_sprite_;
		std::unique_ptr<rendering::ShaderBoldLine::Param> param_bold_line_;
		std::unique_ptr<rendering::ShaderExplodeVolumesGenerateShadows::Param> param_volume_generate_shadows_;

		std::unique_ptr<rendering::ShaderExplodeVolumesLine::Param> param_volume_line_;

		std::vector<std::unique_ptr<rendering::ShaderParam>> params_volumes_;

		bool render_vertices_;
		bool render_edges_;
		bool render_volumes_;
		bool render_volume_lines_;
		bool cast_shadow_;
		bool receive_shadows_;
		bool smooth_volume_faces_;

		AttributePerCell color_per_cell_;
		ColorType color_type_;

		float32 vertex_scale_factor_;
		float32 vertex_base_size_;

		bool auto_update_volume_scalar_min_max_;

		bool clipping_plane_;
		bool clip_only_volumes_;
		rendering::FrameManipulator frame_manipulator_;
		bool show_frame_manipulator_;
		bool manipulating_frame_;
		rendering::Transfo3d transfo_;
		rendering::Transfo3d transfo_scene_;
	};

public:
	VolumeRender(const App& app)
		: ViewModule(app, "VolumeRender (" + std::string{mesh_traits<MESH>::name} + ")"),
		  selected_view_(app.current_view()), selected_mesh_(nullptr)
	{
		outline_engine_ = rendering::Outliner::instance();
		// compute_volume_center_engine_ = std::make_unique<rendering::ComputeVolumeCenterEngine>();
	}

	~VolumeRender()
	{
	}

private:
	void init_mesh(MESH* m)
	{
		MeshData<MESH>& md = mesh_provider_->mesh_data(*m);

		for (View* v : linked_views_)
		{
			Parameters& p = parameters_[v][m];
			cgogn::rendering::ShadowData* shadata = &(v->get_shadow());

			p.init_shaders(shadata);

			p.volume_center_ = add_attribute<Vec3, Volume>(*m, "__volume_center");
			p.volume_clipping_position_ = add_attribute<Vec3, Volume>(*m, "__volume_clipping_position");

			std::shared_ptr<Attribute<Vec3>> vertex_position = get_attribute<Vec3, Vertex>(*m, "position");
			if (vertex_position)
				set_vertex_position(*v, *m, vertex_position);

			mesh_connections_[m].push_back(
				boost::synapse::connect<typename MeshProvider<MESH>::connectivity_changed>(m, [this, v, m]() {
					Parameters& p = parameters_[v][m];
					if (p.vertex_position_)
					{
						p.vertex_base_size_ = float32(geometry::mean_edge_length(*m, p.vertex_position_.get()) / 7.0);
						update_volume_center(*v, *m);
						update_volume_clipping_position(*v, *m);
					}
					v->request_update();
				}));
			mesh_connections_[m].push_back(
				boost::synapse::connect<typename MeshProvider<MESH>::template attribute_changed_t<Vec3>>(
					m, [this, v, m](Attribute<Vec3>* attribute) {
						Parameters& p = parameters_[v][m];
						if (p.vertex_position_.get() == attribute)
						{
							p.vertex_base_size_ =
								float32(geometry::mean_edge_length(*m, p.vertex_position_.get()) / 7.0);
							update_volume_center(*v, *m);
							update_volume_clipping_position(*v, *m);
						}
						v->request_update();
					}));
			mesh_connections_[m].push_back(
				boost::synapse::connect<typename MeshProvider<MESH>::template attribute_changed_t<Scalar>>(
					m, [this, v, m](Attribute<Scalar>* attribute) {
						Parameters& p = parameters_[v][m];
						if (p.volume_scalar_.get() == attribute)
						{
							if (p.auto_update_volume_scalar_min_max_)
								update_volume_scalar_min_max_values(p);
						}
						v->request_update();
					}));

			v->get_light().update();
		}
	}

public:
	void set_vertex_position(View& v, const MESH& m, const std::shared_ptr<Attribute<Vec3>>& vertex_position)
	{
		Parameters& p = parameters_[&v][&m];
		MeshData<MESH>& md = mesh_provider_->mesh_data(m);

		if (p.vertex_position_ == vertex_position)
			return;

		p.vertex_position_ = vertex_position;
		if (p.vertex_position_)
		{
			p.vertex_position_vbo_ = md.update_vbo(vertex_position.get(), true);
			p.vertex_base_size_ = float32(geometry::mean_edge_length(m, vertex_position.get()) / 7.0);
			update_volume_center(v, m);

			if (!p.vertex_clipping_position_)
			{
				p.vertex_clipping_position_ = p.vertex_position_;
				p.vertex_clipping_position_vbo_ = md.update_vbo(p.vertex_clipping_position_.get(), true);
				update_volume_clipping_position(v, m);
			}
		}
		else
		{
			p.vertex_clipping_position_ = nullptr;

			p.vertex_position_vbo_ = nullptr;
			p.vertex_clipping_position_vbo_ = nullptr;
			p.volume_clipping_position_vbo_ = nullptr;
		}

		p.param_point_sprite_->set_vbos({p.vertex_position_vbo_, p.vertex_clipping_position_vbo_});
		p.param_bold_line_->set_vbos({p.vertex_position_vbo_, p.vertex_clipping_position_vbo_});

		p.param_volume_generate_shadows_->set_vbos(
			{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		p.param_volume_line_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		for (int i = 0; i < 4; ++i)
			p.params_volumes_[i]->set_vbos(
				{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		for (int i = 4; i < 8; ++i)
			p.params_volumes_[i]->set_vbos(
				{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_, p.volume_clipping_position_vbo_});

		for (int i = 8; i < 12; ++i)
			p.params_volumes_[i]->set_vbos(
				{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_scalar_vbo_, p.volume_clipping_position_vbo_});

		Scalar size = (md.bb_max_ - md.bb_min_).norm() / 25;
		Vec3 position = 0.2 * md.bb_min_ + 0.8 * md.bb_max_;
		p.frame_manipulator_.set_size(size);
		p.frame_manipulator_.set_position(position);

		v.request_update();
	}

	void set_vertex_clipping_position(View& v, const MESH& m,
									  const std::shared_ptr<Attribute<Vec3>>& vertex_clipping_position)
	{
		Parameters& p = parameters_[&v][&m];
		MeshData<MESH>& md = mesh_provider_->mesh_data(m);

		if (p.vertex_clipping_position_ == vertex_clipping_position)
			return;

		p.vertex_clipping_position_ = vertex_clipping_position;
		if (p.vertex_clipping_position_)
		{
			p.vertex_clipping_position_vbo_ = md.update_vbo(p.vertex_clipping_position_.get(), true);
			update_volume_clipping_position(v, m);
		}
		else if (p.vertex_position_)
		{
			p.vertex_clipping_position_ = p.vertex_position_;
			p.vertex_clipping_position_vbo_ = md.update_vbo(p.vertex_clipping_position_.get(), true);
			update_volume_clipping_position(v, m);
		}
		else
		{
			p.vertex_clipping_position_vbo_ = nullptr;
			p.volume_clipping_position_vbo_ = nullptr;
		}

		p.param_point_sprite_->set_vbos({p.vertex_position_vbo_, p.vertex_clipping_position_vbo_});
		p.param_bold_line_->set_vbos({p.vertex_position_vbo_, p.vertex_clipping_position_vbo_});

		p.param_volume_line_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		p.param_volume_generate_shadows_->set_vbos(
			{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		for (int i = 0; i < 4; ++i)
			p.params_volumes_[i]->set_vbos(
				{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		for (int i = 4; i < 8; ++i)
			p.params_volumes_[i]->set_vbos(
				{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_, p.volume_clipping_position_vbo_});

		for (int i = 8; i < 12; ++i)
			p.params_volumes_[i]->set_vbos(
				{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_scalar_vbo_, p.volume_clipping_position_vbo_});

		v.request_update();
	}

	void set_volume_color(View& v, const MESH& m, const std::shared_ptr<Attribute<Vec3>>& volume_color)
	{
		Parameters& p = parameters_[&v][&m];
		if (p.volume_color_ == volume_color)
			return;

		p.volume_color_ = volume_color;
		if (p.volume_color_)
		{
			MeshData<MESH>& md = mesh_provider_->mesh_data(m);
			p.volume_color_vbo_ = md.update_vbo(p.volume_color_.get(), true);
		}
		else
			p.volume_color_vbo_ = nullptr;

		for (int i = 4; i < 8; ++i)
		{
			p.params_volumes_[i]->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_});
			//			p.params_volumes_[i]->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_,
			//p.volume_color_vbo_});
		}

		v.request_update();
	}

	void set_volume_scalar(View& v, const MESH& m, const std::shared_ptr<Attribute<Scalar>>& volume_scalar)
	{
		Parameters& p = parameters_[&v][&m];
		if (p.volume_scalar_ == volume_scalar)
			return;

		p.volume_scalar_ = volume_scalar;
		if (p.volume_scalar_)
		{
			MeshData<MESH>& md = mesh_provider_->mesh_data(m);
			p.volume_scalar_vbo_ = md.update_vbo(p.volume_scalar_.get(), true);
			if (p.auto_update_volume_scalar_min_max_)
				update_volume_scalar_min_max_values(p);
		}
		else
		{
			p.volume_scalar_vbo_ = nullptr;
			p.data_.color_map_.min_value_ = 0.0f;
			p.data_.color_map_.max_value_ = 1.0f;
			p.data_.color_map_.min_value_ = 0.0f;
			p.data_.color_map_.max_value_ = 1.0f;
		}

		for (int i = 8; i < 12; ++i)
		{
			p.params_volumes_[i]->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_scalar_vbo_});
			//			p.params_volumes_[i]->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_,
			//p.volume_scalar_vbo_});
		}
	}

	void set_volume_explode(View& v, const MESH& m, float expl)
	{
		Parameters& p = parameters_[&v][&m];
		p.data_.explode_ = expl;
	}

protected:
	void update_volume_scalar_min_max_values(Parameters& p)
	{
		Scalar min = std::numeric_limits<float64>::max();
		Scalar max = std::numeric_limits<float64>::lowest();
		for (const Scalar& v : *p.volume_scalar_)
		{
			if (v < min)
				min = v;
			if (v > max)
				max = v;
		}
		p.data_.color_map_.min_value_ = min;
		p.data_.color_map_.max_value_ = max;

		p.data_.color_map_.min_value_ = min;
		p.data_.color_map_.max_value_ = max;
	}

	void update_volume_center(View& v, const MESH& m)
	{
		Parameters& p = parameters_[&v][&m];
		MeshData<MESH>& md = mesh_provider_->mesh_data(m);
		if (p.vertex_position_)
		{
			geometry::compute_centroid<Vec3, Volume>(m, p.vertex_position_.get(), p.volume_center_.get());
			p.volume_center_vbo_ = md.update_vbo(p.volume_center_.get(), true);
		}
	}

	void update_volume_clipping_position(View& v, const MESH& m)
	{

		Parameters& p = parameters_[&v][&m];
		MeshData<MESH>& md = mesh_provider_->mesh_data(m);

		if (p.vertex_clipping_position_)
		{
			geometry::compute_centroid<Vec3, Volume>(m, p.vertex_clipping_position_.get(),
													 p.volume_clipping_position_.get());
			p.volume_clipping_position_vbo_ = md.update_vbo(p.volume_clipping_position_.get(), true);
		}
	}

	void init() override
	{
		mesh_provider_ = static_cast<ui::MeshProvider<MESH>*>(
			app_.module("MeshProvider (" + std::string{mesh_traits<MESH>::name} + ")"));
		mesh_provider_->foreach_mesh([this](MESH& m, const std::string&) { init_mesh(&m); });
		connections_.push_back(boost::synapse::connect<typename MeshProvider<MESH>::mesh_added>(
			mesh_provider_, this, &VolumeRender<MESH>::init_mesh));

		start_timer = std::chrono::high_resolution_clock::now();
	}

public:
	void draw_shadowmap(View* view, const GLMat4d& mat_proj, const GLMat4d& mat_view) override
	{
		for (auto& [m, p] : parameters_[view])
		{
			if (p.render_volumes_ & p.cast_shadow_)
			{
				auto& md = mesh_provider_->mesh_data(*m);
				GLMat4d trf_mesh = p.transfo_.matrix();
				GLMat4d trf_scene = p.transfo_.matrix();
				p.param_volume_generate_shadows_->bind(mat_proj, mat_view * trf_scene * trf_mesh);
				md.draw(rendering::VOLUMES_FACES, p.vertex_position_);
				p.param_volume_generate_shadows_->release();
			}
		}
	}

	void draw(View* view) override
	{
		auto* cv = app_.current_view();
		if (cv== nullptr)
		{
		std::cerr << "WARNING CurrentView NULL" << std::endl;
		return;
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		for (auto& [m, p] : parameters_[view])
		{
			MeshData<MESH>& md = mesh_provider_->mesh_data(*m);
			// int32 indss = (p.smooth_volume_faces_ ? 2 : 0) + ((view->get_shadow().is_started() && p.receive_shadows_)
			// ? 1 : 0);
			int32 indss = (p.smooth_volume_faces_ ? 2 : 0);

			const rendering::GLMat4d& proj_matrix = view->projection_matrix_d();
			const rendering::GLMat4d& view_matrix = view->modelview_matrix_d();

			//			const auto& md = mesh_provider_->mesh_data(*m);
			GLMat4d trf_mesh = p.transfo_.matrix();
			GLMat4d trf_scene = p.transfo_scene_.matrix();

			GLMat4d model_view_matrix = view_matrix * trf_scene * trf_mesh;

			if (p.render_volumes_)
			{
				p.data_.light_position_ = view->get_light().getEyeCoord();

				int32 index_shader = 0;
				switch (p.color_per_cell_)
				{
				case GLOBAL:
					index_shader = indss;
					break;
				case PER_VOLUME:
					switch (p.color_type_)
					{
					case SCALAR:
						index_shader = 8 + indss;
						break;
					case VECTOR:
						index_shader = 4 + indss;
						break;
					}
					break;
				}

				rendering::ShaderParam* param_vol = p.params_volumes_[index_shader].get();
				if (param_vol->attributes_initialized())
				{
					param_vol->bind(proj_matrix, model_view_matrix);
					if (p.smooth_volume_faces_)
						md.draw(rendering::VOLUMES_SMOOTH_FACES, p.vertex_position_);
					else
						md.draw(rendering::VOLUMES_FACES, p.vertex_position_);
					param_vol->release();
				}

				if (p.render_volume_lines_ && p.param_volume_line_->attributes_initialized())
				{
					p.param_volume_line_->bind(proj_matrix, model_view_matrix);
					md.draw(rendering::VOLUMES_EDGES);
					p.param_volume_line_->release();
				}
			}

			if (p.render_edges_ && p.param_bold_line_->attributes_initialized())
			{
				p.param_bold_line_->bind(proj_matrix, model_view_matrix);
				md.draw(rendering::LINES);
				p.param_bold_line_->release();
			}

			if (p.render_vertices_ && p.param_point_sprite_->attributes_initialized())
			{
				p.param_point_sprite_->point_size_ = p.vertex_base_size_ * p.vertex_scale_factor_;
				p.param_point_sprite_->bind(proj_matrix, model_view_matrix);
				md.draw(rendering::POINTS);
				p.param_point_sprite_->release();
			}

			if (p.show_frame_manipulator_)
				p.frame_manipulator_.draw(true, true, proj_matrix, model_view_matrix);

			float64 remain = md.outlined_until_ - App::frame_time_;
			if (remain > 0 && p.vertex_position_vbo_)
			{
				rendering::GLColor color{0.9f, 0.9f, 0.1f, 1};
				color *= float(remain * 2);
				if (!md.is_primitive_uptodate(rendering::TRIANGLES))
					md.init_primitives(rendering::TRIANGLES);
				outline_engine_->draw(p.vertex_position_vbo_, md.mesh_render(), proj_matrix, model_view_matrix, color);
			}
		}
	}

	void key_press_event(View* view, int32 key_code) override
	{
		if (key_code == GLFW_KEY_C)
		{
			if (view == selected_view_ && selected_mesh_)
			{
				Parameters& p = parameters_[selected_view_][selected_mesh_];
				if (p.show_frame_manipulator_)
					p.manipulating_frame_ = true;
			}
		}
	}

	void key_release_event(View* view, int32 key_code) override
	{
		if (key_code == GLFW_KEY_C)
		{
			if (view == selected_view_ && selected_mesh_)
			{
				Parameters& p = parameters_[selected_view_][selected_mesh_];
				p.manipulating_frame_ = false;
			}
		}
	}

	void mouse_press_event(View* view, int32, int32 x, int32 y) override
	{
		if (view == selected_view_ && selected_mesh_)
		{
			Parameters& p = parameters_[selected_view_][selected_mesh_];
			if (p.manipulating_frame_)
			{
				auto [P, Q] = view->pixel_ray(x, y);
				p.frame_manipulator_.pick(x, y, P, Q);
				view->request_update();
			}
		}
	}

	void mouse_release_event(View* view, int32, int32, int32) override
	{
		if (view == selected_view_ && selected_mesh_)
		{
			Parameters& p = parameters_[selected_view_][selected_mesh_];
			p.frame_manipulator_.release();
			view->request_update();
		}
	}

	void mouse_move_event(View* view, int32 x, int32 y) override
	{
		if (view == selected_view_ && selected_mesh_)
		{
			Parameters& p = parameters_[selected_view_][selected_mesh_];
			bool leftpress = view->mouse_button_pressed(GLFW_MOUSE_BUTTON_LEFT);
			bool rightpress = view->mouse_button_pressed(GLFW_MOUSE_BUTTON_RIGHT);
			if (p.manipulating_frame_ && (rightpress || leftpress))
			{
				p.frame_manipulator_.drag(leftpress, x, y);
				if (p.clipping_plane_)
				{
					Vec3 position;
					p.frame_manipulator_.get_position(position);
					Vec3 axis_z;
					p.frame_manipulator_.get_axis(rendering::FrameManipulator::Zt, axis_z);
					float32 d = -(position.dot(axis_z));
					rendering::GLVec4 plane = rendering::construct_GLVec4(axis_z.x(), axis_z.y(), axis_z.z(), d);
					p.data_.plane_clip_ = plane;
				}
				view->stop_event();
				view->request_update();
			}
		}
	}

	void left_panel() override
	{

		bool need_update = false;

		if (app_.nb_views() > 1)
			imgui_view_selector(this, selected_view_, [&](View* v) { selected_view_ = v; });

		imgui_mesh_selector(mesh_provider_, selected_mesh_, "Volume", [&](MESH& m) {
			selected_mesh_ = &m;
			mesh_provider_->mesh_data(m).outlined_until_ = App::frame_time_ + 1.0;
		});

		if (selected_view_)
		{
			ImGui::SetWindowSize(ImVec2(selected_view_->viewport_width() * 0.25f, 0));

			ImGui::LabelText("fps", "%5f", float(app_.fps()));
			ImGui::Separator();
			if (ImGui::SliderFloat("Zplane", &selected_view_->shift_zplane_, -0.2f, 0.9f))
				need_update = true;
			ImGui::Separator();
			if (ImGui::SliderFloat("HBAO Radius", &selected_view_->HBAO_radius_ratio(), 0.0f, 0.3f))
				need_update = true;

			//if (ImGui::SliderInt("HBAO blur", &selected_view_->nb_ao_blurs_, 1, 20))
			//	need_update = true;

			if (ImGui::SliderFloat("Shadow strength", selected_view_->ao_strength_ptr(), 0.f, 5.0f))
				need_update = true;

			//if (ImGui::SliderFloat("ShadowMap bias mult", &selected_view_->bias_k_div_[0], 1.0f, 15.0f))
			//	need_update = true;


			auto& sha_data = selected_view_->get_shadow();
			bool shsta = sha_data.is_started();
			if (ImGui::Checkbox("Shadows", &shsta))
			{
				if (shsta)
					selected_view_->start_shadow();
				else
					selected_view_->stop_shadow();
				need_update = true;
			}
			if (shsta)
			{
				if (ImGui::SliderInt("samples", &sha_data.nb_samples_, 1, 9))
					need_update = true;

				ImGui::Separator();
				LightData& LD = selected_view_->get_light();
				if (ImGui::Checkbox("Cam Light ?", &LD.light_on_cam_))
					need_update = true;

				if (LD.light_on_cam_)
				{
					if (ImGui::SliderFloat("Azimut", &(LD.eye_polar_[0]), float(0.2 * M_PI), float(0.8 * M_PI)) ||
						ImGui::SliderFloat("Elevation", &(LD.eye_polar_[1]), float(-0.25 * M_PI), float(0.25 * M_PI)))
					{
						LD.update();
						need_update = true;
					}
				}
				else
				{
					if (ImGui::SliderFloat("Azimut", &(LD.world_polar_[0]), float(-2.5 * M_PI), float(3.5 * M_PI)) ||
						ImGui::SliderFloat("Elevation", &(LD.world_polar_[1]), float(0.1 * M_PI), float(0.5 * M_PI)))
					{
						LD.update();
						need_update = true;
					}
				}

				ImGui::Separator();

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

						// View* view = app_.current_view();
						// ViewParameters& vp = view_parameters_[view];

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
							rendering::GLVec4 plane =
								rendering::construct_GLVec4(axis_z.x(), axis_z.y(), axis_z.z(), d);

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
							need_update |=
								ImGui::ColorEdit3("Volume color", p.data_.color_.data(), ImGuiColorEditFlags_NoInputs);
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
								need_update |= ImGui::InputFloat("Scalar min##volumecolor",
																 &p.data_.color_map_.min_value_, 0.01f, 1.0f, "%.3f");
								need_update |= ImGui::InputFloat("Scalar max##volumecolor",
																 &p.data_.color_map_.max_value_, 0.01f, 1.0f, "%.3f");
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

					// ViewParameters& vp = view_parameters_[selected_view_];
					// if (vp.use_shadows_)
					//	selected_view_->request_update();
				}

				if (need_update)
					// selected_view_->request_update();
					for (View* v : linked_views_)
						v->request_update();
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
		std::vector<std::shared_ptr<boost::synapse::connection>> connections_;
		std::unordered_map<const MESH*, std::vector<std::shared_ptr<boost::synapse::connection>>> mesh_connections_;
		MeshProvider<MESH>* mesh_provider_;
		rendering::Outliner* outline_engine_;
	};

} // namespace ui

} // namespace cgogn
#endif // CGOGN_MODULE_SURFACE_RENDER_H_
