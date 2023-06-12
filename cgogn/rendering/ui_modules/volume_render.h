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
#include <cgogn/rendering/shaders/shader_fullscreen_texture.h>

#include <cgogn/geometry/algos/centroid.h>
#include <cgogn/geometry/algos/length.h>

#include <GLFW/glfw3.h>
#include <boost/synapse/connect.hpp>

#include <unordered_map>

namespace cgogn
{

namespace ui
{

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

	struct ViewParameters
	{
		std::unique_ptr<rendering::ShaderFullScreenTexture::Param> param_FS_;

		std::shared_ptr<rendering::ShadowData> sha_data_;
		rendering::GLVec3d norm_light_dir_;
		//Camera light_cam_;
		bool use_shadows_;
		std::unique_ptr<rendering::ShaderPlaneShadow::Param> param_plane_;

		ViewParameters() : use_shadows_(false)
		{
			norm_light_dir_ = rendering::GLVec3d{30.0, 10.0, 50.0}.normalized();
			sha_data_ = std::make_shared<rendering::ShadowData>();
			param_plane_ = rendering::ShaderPlaneShadow::generate_param();
			param_plane_->sha_data_ = sha_data_;

			Eigen::Transform<float, 3, Eigen::Affine> trf =
				Eigen::Translation3f(Eigen::Vector3f(0, 0, -4)) * Eigen::Scaling(4.0f);
			param_plane_->transfo_ = trf.matrix();

		}
		
	};


	struct Parameters
	{
		Parameters()
			: vertex_position_(nullptr),
			  vertex_position_vbo_(nullptr), vertex_clipping_position_(nullptr),
			  volume_clipping_position_(nullptr), volume_clipping_position_vbo_(nullptr), volume_scalar_(nullptr),
			  volume_scalar_vbo_(nullptr), volume_color_(nullptr), volume_color_vbo_(nullptr), volume_center_(nullptr),
			  volume_center_vbo_(nullptr), render_vertices_(false), render_edges_(false), render_volumes_(true),
			  render_volume_lines_(true), render_shadow_(false), color_per_cell_(GLOBAL), color_type_(SCALAR),
			  vertex_scale_factor_(1.0),
			  auto_update_volume_scalar_min_max_(true), clipping_plane_(false), clip_only_volumes_(true),
			  show_frame_manipulator_(false), manipulating_frame_(false)
		{
			param_point_sprite_ = rendering::ShaderPointSprite::generate_param();
			param_point_sprite_->color_ = rendering::GLColor(1, 0.5f, 0, 1);

			param_bold_line_ = rendering::ShaderBoldLine::generate_param();
			param_bold_line_->color_ = {1.0f, 1.0f, 1.0f, 1.0f};
			param_bold_line_->width_ = 2.0f;

			param_volume_ = rendering::ShaderExplodeVolumes::generate_param();
			param_volume_smooth_ = rendering::ShaderExplodeVolumesSmooth::generate_param();
			param_volume_generate_shadows_ = rendering::ShaderExplodeVolumesGenerateShadows::generate_param();
			param_volume_shadows_ = rendering::ShaderExplodeVolumesShadows::generate_param();
			param_volume_color_ = rendering::ShaderExplodeVolumesColor::generate_param();
			param_volume_color_smooth_ = rendering::ShaderExplodeVolumesColorSmooth::generate_param();
			param_volume_scalar_ = rendering::ShaderExplodeVolumesScalar::generate_param();
			param_volume_scalar_smooth_ = rendering::ShaderExplodeVolumesScalarSmooth::generate_param();
			param_volume_line_ = rendering::ShaderExplodeVolumesLine::generate_param();

			param_volume_->data_ = &data_;
			param_volume_smooth_->data_ = &data_;
			param_volume_generate_shadows_->data_ = &data_;
			param_volume_shadows_->data_ = &data_;
			param_volume_color_->data_ = &data_;
			param_volume_color_smooth_->data_ = &data_;
			param_volume_line_->data_ = &data_;
	
			data_.color_ = {0.4f, 0.8f, 1.0f, 1.0f};
			data_.color_line_ = {0.0f, 0.0f, 0.0f, 1.0f};

			param_volume_gen_ = param_volume_.get();
			param_volume_color_gen_ = param_volume_color_.get();
			param_volume_scalar_gen_ = param_volume_scalar_.get();
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

		std::unique_ptr<rendering::ShaderPointSprite::Param> param_point_sprite_;
		std::unique_ptr<rendering::ShaderBoldLine::Param> param_bold_line_;
		std::unique_ptr<rendering::ShaderExplodeVolumes::Param> param_volume_;
		std::unique_ptr<rendering::ShaderExplodeVolumesLine::Param> param_volume_line_;
		std::unique_ptr<rendering::ShaderExplodeVolumesColor::Param> param_volume_color_;
		std::unique_ptr<rendering::ShaderExplodeVolumesScalar::Param> param_volume_scalar_;
		std::unique_ptr<rendering::ShaderExplodeVolumesSmooth::Param> param_volume_smooth_;
		std::unique_ptr<rendering::ShaderExplodeVolumesColorSmooth::Param> param_volume_color_smooth_;
		std::unique_ptr<rendering::ShaderExplodeVolumesScalarSmooth::Param> param_volume_scalar_smooth_;

		
		std::unique_ptr<rendering::ShaderExplodeVolumesShadows::Param> param_volume_shadows_;
		std::unique_ptr<rendering::ShaderExplodeVolumesGenerateShadows::Param> param_volume_generate_shadows_;
		
		rendering::ShaderParam* param_volume_gen_;
		rendering::ShaderParam* param_volume_gen_shadows_;
		rendering::ShaderParam* param_volume_color_gen_;
		rendering::ShaderParam* param_volume_scalar_gen_;
	
		bool render_vertices_;
		bool render_edges_;
		bool render_volumes_;
		bool render_volume_lines_;
		bool render_shadow_;

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
	};

public:
	VolumeRender(const App& app)
		: ViewModule(app, "VolumeRender (" + std::string{mesh_traits<MESH>::name} + ")"),
		  selected_view_(app.current_view()), selected_mesh_(nullptr)
	{
		outline_engine_ = rendering::Outliner::instance();
	}

	~VolumeRender()
	{
	}

private:
	void init_mesh(MESH* m)
	{
		for (View* v : linked_views_)
		{
			Parameters& p = parameters_[v][m];
			ViewParameters& vp = view_parameters_[v];
			if (vp.sha_data_ == nullptr)
			{

				vp.norm_light_dir_ = rendering::GLVec3d{10.0, 1.0, 50.0}.normalized();
				vp.sha_data_ = std::make_shared<rendering::ShadowData>();
				//vp.param_FS_ = rendering::ShaderFullScreenTexture::generate_param();
			}
				
			p.param_volume_shadows_->sha_data_ = vp.sha_data_;

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

		p.param_volume_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_smooth_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		p.param_volume_shadows_->set_vbos(
			{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_generate_shadows_->set_vbos(
			{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		p.param_volume_line_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_color_->set_vbos(
			{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_color_smooth_->set_vbos(
			{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_, p.volume_clipping_position_vbo_});

		p.param_volume_scalar_->set_vbos(
			{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_scalar_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_scalar_smooth_->set_vbos(
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

		p.param_volume_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_smooth_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});

		p.param_volume_line_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_color_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_color_smooth_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_, p.volume_clipping_position_vbo_});



		p.param_volume_scalar_->set_vbos(
			{p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_scalar_vbo_, p.volume_clipping_position_vbo_});
		p.param_volume_scalar_smooth_->set_vbos(
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

		p.param_volume_color_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_});
		p.param_volume_color_smooth_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_color_vbo_});

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
			p.param_volume_scalar_->data_->color_map_.min_value_ = 0.0f;
			p.param_volume_scalar_->data_->color_map_.max_value_ = 1.0f;
			p.param_volume_scalar_smooth_->data_->color_map_.min_value_ = 0.0f;
			p.param_volume_scalar_smooth_->data_->color_map_.max_value_ = 1.0f;
		}

		p.param_volume_scalar_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_scalar_vbo_});
		p.param_volume_scalar_smooth_->set_vbos({p.vertex_position_vbo_, p.volume_center_vbo_, p.volume_scalar_vbo_});
	}

	void set_volume_explode(View& v, const MESH& m, float expl)
	{
		Parameters& p = parameters_[&v][&m];
		p.data_.explode_ = expl;
		//p.param_volume_line_->explode_ = expl;
		//p.param_volume_color_->data_->explode_ = expl;
		//p.param_volume_scalar_->data_->explode_ = expl;
		//p.param_volume_smooth_->data_->explode_ = expl;
		//p.param_volume_color_smooth_->data_->explode_ = expl;
		//p.param_volume_scalar_smooth_->data_->explode_ = expl;
	}

	void set_smoothing(const MESH& m, bool smo)
	{
		MeshData<MESH>& md = mesh_provider_->mesh_data(m);
		if(smo)
			for (auto& vp : parameters_)
			{
				vp.second[&m].param_volume_gen_ = vp.second[&m].param_volume_smooth_.get();
				vp.second[&m].param_volume_color_gen_ = vp.second[&m].param_volume_color_smooth_.get();
				vp.second[&m].param_volume_scalar_gen_ = vp.second[&m].param_volume_scalar_smooth_.get();
			}
				
		else
			for (auto& vp : parameters_)
			{
				vp.second[&m].param_volume_gen_ = vp.second[&m].param_volume_.get();
				vp.second[&m].param_volume_color_gen_ = vp.second[&m].param_volume_color_.get();
				vp.second[&m].param_volume_scalar_gen_ = vp.second[&m].param_volume_scalar_.get();
			}

		md.mesh_render()->set_smooth_volume_faces(smo);
		md.mesh_render()->set_primitive_dirty(rendering::DrawingType::VOLUMES_FACES);
	}

	//void set_light_dir(const MESH& m, const Eigen::Vector3f& LD)
	//{
	//	parameters_[current_view][&m].light_dir = LD;
	//	view_parameters_[current_view].data_.light_dir = LD;
	//}

	void set_shadows(View* view, bool on_off)
	{
		if (on_off)
		{
			//view->camera().set_type(Camera::Type::ORTHOGRAPHIC);
			view_parameters_[view].sha_data_->start(2048);
		}
		else
		{
			//view->camera().set_type(Camera::Type::PERSPECTIVE);
			view_parameters_[view].sha_data_->stop();
		}
			
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


	}

	void draw(View* view) override
	{
		ViewParameters& vp = view_parameters_[app_.current_view()];
		if (vp.use_shadows_)
		{
			const Camera& cam = app_.current_view()->camera();
			float64 sd = 64.0 * cam.scene_radius(); 
			float64 sr = cam.scene_radius(); //*0.9 ?

			rendering::GLVec3d light_position = cam.pivot_point() + sd * vp.norm_light_dir_;

			auto orthographic = [&]() {
				float64 hw = 1.0 / sr;
				rendering::GLMat4d m;
				m << hw, 0, 0, 0, 0, hw, 0, 0, 0, 0, -hw, -sd*hw, 0, 0, 0, 1;
				return m;
			};

			rendering::GLMat4d light_projection_matrix = orthographic();

			// warning dir & up mus be normalized
			auto look_dir = [](const rendering::GLVec3d& eye, const rendering::GLVec3d& dir,
							   const rendering::GLVec3d& up) {
				rendering::GLVec3d zAxis = -dir; //.normalized();
				//rendering::GLVec3d xAxis = up.normalized().cross(zAxis).normalized();
				rendering::GLVec3d xAxis = up.cross(zAxis).normalized();
				rendering::GLVec3d yAxis = zAxis.cross(xAxis).normalized();

				rendering::GLMat4d trf;
				trf.block<1, 3>(0, 0) = xAxis.transpose();
				trf.block<1, 3>(1, 0) = yAxis.transpose();
				trf.block<1, 3>(2, 0) = zAxis.transpose();

				trf.block<3, 1>(0, 3) = rendering::GLVec3d(-xAxis.dot(eye), -yAxis.dot(eye), -zAxis.dot(eye));
				trf.block<1, 4>(3, 0) = rendering::GLVec4d(0, 0, 0, 1).transpose();

				return trf;
			};

			rendering::GLMat4d light_view_matrix =
				look_dir(light_position, - vp.norm_light_dir_, rendering::GLVec3d(0, 0, 1));
			
			vp.sha_data_->fbo_shadows_->bind();
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);

			double wh = cam.scene_radius();	

			rendering::GLMat4d biasmat;
			biasmat << 0.5 , 0.0 , 0.0 , 0.5 ,
				0.0 , 0.5 , 0.0 , 0.5 ,
				0.0 , 0.0 , 0.5 , 0.5 ,
				0.0 , 0.0 , 0.0 , 1.0;

			vp.sha_data_->shadow_matrix_ =
				biasmat * light_projection_matrix * light_view_matrix;

			rendering::GLVec3d lpv = (view->modelview_matrix_d() *
									  rendering::GLVec4d(light_position[0], light_position[1], light_position[2], 1.0))
										 .block<3, 1>(0, 0);

			//std::cout << lpv.transpose() << std::endl;
			vp.sha_data_->light_position_ = lpv;

			for (auto& [m, p] : parameters_[view])
			{
				if (p.render_volumes_)
				{
					p.data_.light_position_ = lpv.cast<float>();
					glEnable(GL_POLYGON_OFFSET_FILL);
					glPolygonOffset(1.0f, 1.5f);

					p.param_volume_generate_shadows_->bind(light_projection_matrix.cast<float>(),
														   light_view_matrix.cast<float>());
					mesh_provider_->mesh_data(*m).draw(rendering::VOLUMES_FACES, p.vertex_position_);
					p.param_volume_generate_shadows_->release();
				}
			}
			vp.sha_data_->fbo_shadows_->release();

			glDisable(GL_CULL_FACE);
		// IF  USEPLANE ...
			vp.param_plane_->draw(view->projection_matrix(), view->modelview_matrix());

		}


		for (auto& [m, p] : parameters_[view])
		{
			MeshData<MESH>& md = mesh_provider_->mesh_data(*m);

			const rendering::GLMat4& proj_matrix = view->projection_matrix();
			const rendering::GLMat4& view_matrix = view->modelview_matrix();

			if (p.render_volumes_)
			{
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(1.0f, 1.5f);

				switch (p.color_per_cell_)
				{
				case GLOBAL: {
					if (vp.use_shadows_)
					{
						if (p.param_volume_shadows_->attributes_initialized())
						{
							std::cout << "======= BIAS MAT ======" << std::endl;
							std::cout << vp.sha_data_->shadow_matrix_ << std::endl;

							p.param_volume_shadows_->bind(proj_matrix, view_matrix);
							md.draw(rendering::VOLUMES_FACES, p.vertex_position_);
							p.param_volume_shadows_->release();
						}
					}
					else
					{
						if (p.param_volume_gen_->attributes_initialized())
						{
							p.param_volume_gen_->bind(proj_matrix, view_matrix);
							md.draw(rendering::VOLUMES_FACES, p.vertex_position_);
							p.param_volume_gen_->release();
						}
					}
				}
				break;
				case PER_VOLUME: {
					switch (p.color_type_)
					{
					case SCALAR: {
						if (p.param_volume_scalar_gen_->attributes_initialized())
						{
							p.param_volume_scalar_gen_->bind(proj_matrix, view_matrix);
							md.draw(rendering::VOLUMES_FACES, p.vertex_position_);
							p.param_volume_scalar_gen_->release();
						}
					}
					break;
					case VECTOR: {
						if (p.param_volume_color_gen_->attributes_initialized())
						{
							p.param_volume_color_gen_->bind(proj_matrix, view_matrix);
							md.draw(rendering::VOLUMES_FACES, p.vertex_position_);
							p.param_volume_color_gen_->release();
						}
					}
					break;
					}
				}
				break;
				}

				glDisable(GL_POLYGON_OFFSET_FILL);

				if (p.render_volume_lines_ && p.param_volume_line_->attributes_initialized())
				{
					p.param_volume_line_->bind(proj_matrix, view_matrix);
					md.draw(rendering::VOLUMES_EDGES);
					p.param_volume_line_->release();
				}
			}

			if (p.render_edges_ && p.param_bold_line_->attributes_initialized())
			{
				p.param_bold_line_->bind(proj_matrix, view_matrix);
				md.draw(rendering::LINES);
				p.param_bold_line_->release();
			}

			if (p.render_vertices_ && p.param_point_sprite_->attributes_initialized())
			{
				p.param_point_sprite_->point_size_ = p.vertex_base_size_ * p.vertex_scale_factor_;
				p.param_point_sprite_->bind(proj_matrix, view_matrix);
				md.draw(rendering::POINTS);
				p.param_point_sprite_->release();
			}

			if (p.show_frame_manipulator_)
				p.frame_manipulator_.draw(true, true, proj_matrix, view_matrix);

			float64 remain = md.outlined_until_ - App::frame_time_;
			if (remain > 0 && p.vertex_position_vbo_)
			{
				rendering::GLColor color{0.9f, 0.9f, 0.1f, 1};
				color *= float(remain * 2);
				if (!md.is_primitive_uptodate(rendering::TRIANGLES))
					md.init_primitives(rendering::TRIANGLES);
				outline_engine_->draw(p.vertex_position_vbo_, md.mesh_render(), proj_matrix, view_matrix, color);
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

		if (selected_view_ && selected_mesh_)
		{
			Parameters& p = parameters_[selected_view_][selected_mesh_];

			imgui_combo_attribute<Vertex, Vec3>(*selected_mesh_, p.vertex_position_, "Position",
												[&](const std::shared_ptr<Attribute<Vec3>>& attribute) {
													set_vertex_position(*selected_view_, *selected_mesh_, attribute);
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
				need_update |=
					ImGui::ColorEdit3("Color##edges", p.param_bold_line_->color_.data(), ImGuiColorEditFlags_NoInputs);
				need_update |= ImGui::SliderFloat("Width##edges", &p.param_bold_line_->width_, 1.0f, 10.0f);
			}

			ImGui::Separator();
			need_update |= ImGui::Checkbox("Volumes", &p.render_volumes_);
			if (p.render_volumes_)
			{
				bool& svf = mesh_provider_->mesh_data(*selected_mesh_).mesh_render()->ref_smooth_volume_faces();
				if (ImGui::Checkbox("Smooth faces", &svf))
				{
					set_smoothing(*selected_mesh_, svf);
					need_update = true;
				}
				if (ImGui::SliderFloat("Explode", &p.data_.explode_, 0.01f, 1.0f))
				{
					need_update = true;
				}

				View* view = app_.current_view();
				ViewParameters& vp = view_parameters_[view];
				if (ImGui::Checkbox("Shadows", &vp.use_shadows_))
				{
					set_shadows(view, vp.use_shadows_);
					need_update = true;
				}
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
					need_update |=
						ImGui::ColorEdit3("Volume color", p.param_volume_->data_->color_.data(), ImGuiColorEditFlags_NoInputs);
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
														 &p.data_.color_map_.min_value_,
											  0.01f, 1.0f, "%.3f");
						need_update |= ImGui::InputFloat("Scalar max##volumecolor",
														 &p.data_.color_map_.max_value_,
											  0.01f, 1.0f, "%.3f");
						if (ImGui::Checkbox("Auto update min/max##volumecolor", &p.auto_update_volume_scalar_min_max_))
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
			}

			float64 remain = mesh_provider_->mesh_data(*selected_mesh_).outlined_until_ - App::frame_time_;
			if (remain > 0)
				need_update = true;

			if (need_update)
				for (View* v : linked_views_)
					v->request_update();
		}
	}

private:
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
