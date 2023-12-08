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

#ifndef CGOGN_MODULE_SURFACE_OBJ_RENDER_H_
#define CGOGN_MODULE_SURFACE_OBJ_RENDER_H_ 

#include <cgogn/core/types/maps/cmap/cmap2.h>

#include <cgogn/core/ui_modules/mesh_provider.h>
#include <cgogn/ui/app.h>
#include <cgogn/ui/imgui_helpers.h>
#include <cgogn/ui/module.h>
#include <cgogn/ui/view.h>
#include <cgogn/geometry/types/vector_traits.h>

#include <cgogn/rendering/shaders/shader_obj_flat_texture.h>
#include <cgogn/rendering/shaders/shader_obj_meshuv.h>
#include <cgogn/rendering/shaders/shader_mesh_2d_edges.h>
#include <cgogn/rendering/texture.h>

#include <boost/synapse/connect.hpp>
#include <unordered_map>

namespace cgogn
{
using geometry::Scalar;
using geometry::Vec2;
using geometry::Vec3;

enum OBJBoundaries
{
	BoundaryPos = 0,
	BoundaryTC,
	BoundaryNONE
};

struct MeshObjTriplet
{
	CMap2 map_pos_;
	CMap2 map_tc_;
	CMap2 map_no_;
};


namespace ui
{



	template <typename MESH>
class MeshProvider : public ProviderModule
{
	template <typename T>
	using Attribute = typename mesh_traits<MESH>::template Attribute<T>;
	using AttributeGen = typename mesh_traits<MESH>::AttributeGen;

	using Vertex = typename mesh_traits<MESH>::Vertex;

public:
	MeshProvider(const App& app)
		: ProviderModule(app, "MeshProvider (" + std::string{mesh_traits<MESH>::name} + ")"), selected_mesh_(nullptr),
		  bb_min_(0, 0, 0), bb_max_(0, 0, 0)
	{
		// for (auto& n : new_attribute_name_)
		// 	n[0] = '\0';
		if constexpr (mesh_traits<MESH>::dimension == 1)
			supported_formats_ = &supported_graph_formats_;
		if constexpr (mesh_traits<MESH>::dimension == 2)
			supported_formats_ = &supported_surface_formats_;
		if constexpr (mesh_traits<MESH>::dimension == 3)
			supported_formats_ = &supported_volume_formats_;
	}

	~MeshProvider()
	{
	}

	MESH* add_mesh(const std::string& name)
	{
		if constexpr (std::is_default_constructible_v<MESH>)
		{
			const auto [it, inserted] = meshes_.emplace(name, std::make_unique<MESH>());
			MESH* m = it->second.get();
			if (inserted)
			{
				MeshData<MESH>& md = mesh_data(*m);
				md.init(m);
				boost::synapse::emit<mesh_added>(this, m);
			}
			return m;
		}
		else
			return nullptr;
	}

	MESH* clone_mesh(const MESH& m)
	{
		const std::string& m_name = mesh_name(m);
		std::string name =
			remove_extension(m_name) + "_" + std::to_string(number_of_meshes()) + "." + extension(m_name);
		MESH* result = add_mesh(name);
		copy(*result, m);
		std::shared_ptr<Attribute<Vec3>> vertex_position = get_attribute<Vec3, Vertex>(*result, "position");
		if (vertex_position)
			set_mesh_bb_vertex_position(*result, vertex_position);
		boost::synapse::emit<mesh_added>(this, result);
		emit_connectivity_changed(*result);
		// TODO: emit attributes changed ?
		return result;
	}

	void register_mesh(MESH* m, const std::string& name)
	{
		const auto [it, inserted] = meshes_.emplace(name, std::unique_ptr<MESH>(m));
		if (inserted)
		{
			MeshData<MESH>& md = mesh_data_[m];
			md.init(m);
			std::shared_ptr<Attribute<Vec3>> vertex_position = get_attribute<Vec3, Vertex>(*m, "position");
			if (vertex_position)
				set_mesh_bb_vertex_position(*m, vertex_position);
			boost::synapse::emit<mesh_added>(this, m);
		}
	}

	void clear_mesh(MESH& m)
	{
		clear(m);
		emit_connectivity_changed(m);
		// TODO: emit attributes changed ?
	}

	void copy_mesh(MESH& dst, const MESH& src)
	{
		copy(dst, src);
		emit_connectivity_changed(dst);
	}

	void remove_mesh(MESH& m)
	{
		// TODO
	}

	bool has_mesh(const std::string& name) const
	{
		return meshes_.count(name) == 1;
	}

	MESH* load_graph_from_file(const std::string& filename)
	{
		std::string name = filename_from_path(filename);
		if (has_mesh(name))
			name = remove_extension(name) + "_" + std::to_string(number_of_meshes()) + "." + extension(name);
		const auto [it, inserted] = meshes_.emplace(name, std::make_unique<MESH>());
		MESH* m = it->second.get();

		std::string ext = extension(filename);
		bool imported;

		if constexpr (mesh_traits<MESH>::dimension == 1 && std::is_default_constructible_v<MESH>)
		{
			if (ext.compare("cg") == 0)
				imported = io::import_CG(*m, filename);
			else if (ext.compare("cgr") == 0)
				imported = io::import_CGR(*m, filename);
			else if (ext.compare("ig") == 0)
				imported = io::import_IG(*m, filename);
			else if (ext.compare("skel") == 0)
				imported = io::import_SKEL(*m, filename);
			else
				imported = false;
		}
		else if constexpr (std::is_same_v<MESH, IncidenceGraph>)
		{
			if (ext.compare("cg") == 0)
				imported = io::import_CG(*m, filename);
			else if (ext.compare("ig") == 0)
				imported = io::import_IG(*m, filename);
			else
				imported = false;
		}

		if (imported)
		{
			MeshData<MESH>& md = mesh_data(*m);
			md.init(m);
			mesh_filename_[m] = filename;
			std::shared_ptr<Attribute<Vec3>> vertex_position = get_attribute<Vec3, Vertex>(*m, "position");
			if (vertex_position)
				set_mesh_bb_vertex_position(*m, vertex_position);
			boost::synapse::emit<mesh_added>(this, m);
			return m;
		}
		else
		{
			meshes_.erase(name);
			return nullptr;
		}
	}

	void save_graph_to_file(MESH& m, const Attribute<Vec3>* vertex_position, const std::string& filetype,
							const std::string& filename)
	{
		if constexpr (mesh_traits<MESH>::dimension == 1)
		{
			if (filetype.compare("cg") == 0)
				io::export_CG(m, vertex_position, filename + ".cg");
			else if (filetype.compare("ig") == 0)
				io::export_IG(m, vertex_position, filename + ".ig");
			// else if (filetype.compare("cgr") == 0)
			// 	// TODO io::export_CGR();
			// else if (filetype.compare("skel") == 0)
			// 	// TODO io::export_SKEL();
		}
	}

	MESH* load_surface_from_file(const std::string& filename, bool normalized = true)
	{
		if constexpr (mesh_traits<MESH>::dimension == 2 && std::is_default_constructible_v<MESH>)
		{
			std::string name = filename_from_path(filename);
			if (has_mesh(name))
				name = remove_extension(name) + "_" + std::to_string(number_of_meshes()) + "." + extension(name);
			const auto [it, inserted] = meshes_.emplace(name, std::make_unique<MESH>());
			MESH* m = it->second.get();

			std::string ext = extension(filename);
			bool imported = false;
			if (ext.compare("off") == 0)
				imported = io::import_OFF(*m, filename);
			else if (ext.compare("obj") == 0)
				imported = io::import_OBJ(*m, filename);
			else if (ext.compare("ply") == 0)
				imported = io::import_PLY(*m, filename);
			else if (ext.compare("ig") == 0)
			{
				if constexpr (std::is_same_v<MESH, IncidenceGraph>)
					imported = io::import_IG(*m, filename);
			}

			if (imported)
			{
				MeshData<MESH>& md = mesh_data(*m);
				md.init(m);
				mesh_filename_[m] = filename;
				std::shared_ptr<Attribute<Vec3>> vertex_position = get_attribute<Vec3, Vertex>(*m, "position");
				if (vertex_position)
				{
					if (normalized)
						geometry::rescale(*vertex_position, 1);
					set_mesh_bb_vertex_position(*m, vertex_position);
				}
				boost::synapse::emit<mesh_added>(this, m);
				return m;
			}
			else
			{
				meshes_.erase(name);
				return nullptr;
			}
		}
		else
			return nullptr;
	}

	void save_surface_to_file(MESH& m, const Attribute<Vec3>* vertex_position, const std::string& filetype,
							  const std::string& filename)
	{
		if constexpr (mesh_traits<MESH>::dimension == 2)
		{
			if (filetype.compare("off") == 0)
				io::export_OFF(m, vertex_position, filename + ".off");
			else if (filetype.compare("ig") == 0)
			{
				if constexpr (has_edge_v<MESH>)
					io::export_IG(m, vertex_position, filename + ".ig");
			}
		}
	}

	MESH* load_volume_from_file(const std::string& filename)
	{
		if constexpr (mesh_traits<MESH>::dimension == 3 && std::is_default_constructible_v<MESH>)
		{
			std::string name = filename_from_path(filename);
			if (has_mesh(name))
				name = remove_extension(name) + "_" + std::to_string(number_of_meshes()) + "." + extension(name);
			const auto [it, inserted] = meshes_.emplace(name, std::make_unique<MESH>());
			MESH* m = it->second.get();

			std::string ext = extension(filename);
			bool imported;
			if (ext.compare("tet") == 0)
				imported = io::import_TET(*m, filename);
			else if (ext.compare("mesh") == 0 || ext.compare("meshb") == 0)
				imported = io::import_MESHB(*m, filename);
			else
				imported = false;

			if (imported)
			{
				MeshData<MESH>& md = mesh_data(*m);
				md.init(m);
				mesh_filename_[m] = filename;
				std::shared_ptr<Attribute<Vec3>> vertex_position = get_attribute<Vec3, Vertex>(*m, "position");
				if (vertex_position)
					set_mesh_bb_vertex_position(*m, vertex_position);
				boost::synapse::emit<mesh_added>(this, m);
				return m;
			}
			else
			{
				meshes_.erase(name);
				return nullptr;
			}
		}
		else
			return nullptr;
	}

	void save_volume_to_file(MESH& m, const Attribute<Vec3>* vertex_position, const std::string& filetype,
							 const std::string& filename)
	{
		if constexpr (mesh_traits<MESH>::dimension == 3)
		{
			if (filetype.compare("mesh") == 0)
				io::export_MESH(m, vertex_position, filename + ".mesh");
			// else if (filetype.compare("cgns") == 0)
			// 	io::export_CGNS(m, vertex_position, filename + ".cgns");

			// else if (filetype.compare("tet") == 0)
			// 	// TODO io::export_TET();
			// else if (filetype.compare("meshb") == 0)
			// 	// TODO io::export_MESHB();
		}
	}

	auto load_surface_from_OBJ_file(const std::string& filename, bool normalized = true)
		-> std::enable_if_t<std::is_same_t<MESH, MeshObjTriplet>, MeshObjTriplet>
	{
		if constexpr (mesh_traits<MESH>::dimension == 2 && std::is_default_constructible_v<MESH>)
		{
			std::string name = filename_from_path(filename);
			if (has_mesh(name))
				name = remove_extension(name) + "_" + std::to_string(number_of_meshes()) + "." + extension(name);
			const auto [it, inserted] = meshes_.emplace(name, std::make_unique<MESH>());
			MESH* m_pos = it->second.get();
			const auto [it2, inserted2] = meshes_.emplace(name + "_tc", std::make_unique<MESH>());
			MESH* m_tc = it2->second.get();
			const auto [it3, inserted3] = meshes_.emplace(name + "_no", std::make_unique<MESH>());
			MESH* m_n = it3->second.get();

			std::string ext = extension(filename);
			bool imported = false;

			if (ext.compare("obj") == 0)
				imported = io::import_OBJ_tn(*m_pos, *m_tc, *m_n, filename);

			if (imported)
			{
				MeshData<MESH>& md = mesh_data(*m_pos);
				md.init(m_pos);
				mesh_filename_[m_pos] = filename;
				std::shared_ptr<Attribute<Vec3>> vertex_position = get_attribute<Vec3, Vertex>(*m_pos, "position");
				if (vertex_position)
				{
					if (normalized)
						geometry::rescale(*vertex_position, 1);
					set_mesh_bb_vertex_position(*m_pos, vertex_position);
				}

				MeshData<MESH>& md_tc = mesh_data(*m_tc);
				md_tc.init(m_tc);
				mesh_filename_[m_tc] = filename + "_tc";
				MeshData<MESH>& md_no = mesh_data(*m_n);
				md_no.init(m_n);
				mesh_filename_[m_n] = filename + "_no";

				boost::synapse::emit<mesh_added>(this, m_pos);

				return {m_pos, m_tc, m_n};
			}
			else
			{
				meshes_.erase(name);
				return {nullptr, nullptr, nullptr};
			}
		}
		else
			return {nullptr, nullptr, nullptr};
	}

	template <typename FUNC>
	void foreach_mesh(const FUNC& f)
	{
		static_assert(is_ith_func_parameter_same<FUNC, 0, MESH&>::value, "Wrong function parameter type");
		static_assert(is_ith_func_parameter_same<FUNC, 1, const std::string&>::value, "Wrong function parameter type");
		for (auto& [name, m] : meshes_)
			f(*m, name);
	}

	inline uint32 number_of_meshes()
	{
		return uint32(meshes_.size());
	}

	std::string mesh_name(const MESH& m) const
	{
		auto it =
			std::find_if(meshes_.begin(), meshes_.end(), [&](const auto& pair) { return pair.second.get() == &m; });
		if (it != meshes_.end())
			return it->first;
		else
			return "";
	}

	std::string mesh_filename(const MESH& m)
	{
		if (mesh_filename_.count(&m) > 0)
			return mesh_filename_[&m];
		else
			return "";
	}

	MeshData<MESH>& mesh_data(const MESH& m)
	{
		return mesh_data_[&m];
	}

	std::pair<Vec3, Vec3> meshes_bb() const override
	{
		return std::make_pair(bb_min_, bb_max_);
	}

private:
	void update_meshes_bb()
	{
		for (uint32 i = 0; i < 3; ++i)
		{
			bb_min_[i] = std::numeric_limits<float64>::max();
			bb_max_[i] = std::numeric_limits<float64>::lowest();
		}
		for (auto& [m, md] : mesh_data_)
		{
			for (uint32 i = 0; i < 3; ++i)
			{
				if (md.bb_min_[i] < bb_min_[i])
					bb_min_[i] = md.bb_min_[i];
				if (md.bb_max_[i] > bb_max_[i])
					bb_max_[i] = md.bb_max_[i];
			}
		}
	}

public:
	void set_mesh_bb_vertex_position(const MESH& m, const std::shared_ptr<Attribute<Vec3>>& vertex_position)
	{
		MeshData<MESH>& md = mesh_data(m);
		md.bb_vertex_position_ = vertex_position;
		md.update_bb();
		update_meshes_bb();
		for (View* v : linked_views_)
		{
			v->update_scene_bb();
			v->show_entire_scene();
		}
	}

	/////////////
	// SIGNALS //
	/////////////

	using mesh_added = struct mesh_added_ (*)(MESH* m);
	template <typename T>
	using attribute_changed_t = struct attribute_changed_t_ (*)(Attribute<T>* attribute);
	using attribute_changed = struct attribute_changed_ (*)(AttributeGen* attribute);
	using connectivity_changed = struct connectivity_changed_ (*)();
	template <typename CELL>
	using cells_set_changed = struct cells_set_changed_ (*)(CellsSet<MESH, CELL>* set);

	template <typename T>
	void emit_attribute_changed(const MESH& m, Attribute<T>* attribute)
	{
		MeshData<MESH>& md = mesh_data(m);
		md.update_vbo(attribute);
		if (static_cast<AttributeGen*>(md.bb_vertex_position_.get()) == static_cast<AttributeGen*>(attribute))
		{
			md.update_bb();
			update_meshes_bb();
			for (View* v : linked_views_)
				v->update_scene_bb();
		}

		for (View* v : linked_views_)
			v->request_update();

		boost::synapse::emit<attribute_changed>(&m, attribute);
		boost::synapse::emit<attribute_changed_t<T>>(&m, attribute);
	}

	void emit_connectivity_changed(const MESH& m)
	{
		MeshData<MESH>& md = mesh_data(m);
		md.update_nb_cells();
		md.rebuild_cells_sets();
		md.set_all_primitives_dirty();

		for (View* v : linked_views_)
			v->request_update();

		boost::synapse::emit<connectivity_changed>(&m);
	}

	template <typename CELL>
	void emit_cells_set_changed(const MESH& m, CellsSet<MESH, CELL>* set)
	{
		boost::synapse::emit<cells_set_changed<CELL>>(&m, set);
	}

protected:
	void main_menu() override
	{
		static std::shared_ptr<pfd::open_file> open_file_dialog;
		if (open_file_dialog && open_file_dialog->ready())
		{
			auto result = open_file_dialog->result();
			if (uint32(result.size()) > 0)
			{
				for (auto file : result)
					load_surface_from_obj_file(file);
			}
			open_file_dialog = nullptr;
		}

		open_save_popup_ = false;
		if (ImGui::BeginMenu(name_.c_str()))
		{
			if (ImGui::MenuItem("Add mesh"))
				add_mesh(std::string{mesh_traits<MESH>::name});
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, (bool)open_file_dialog);
			if (ImGui::MenuItem("Load mesh"))
			{
				open_file_dialog = std::make_shared<pfd::open_file>("Choose file", ".", supported_surface_files_,
																		pfd::opt::multiselect);
			}
			ImGui::PopItemFlag();
			if (ImGui::MenuItem("Save mesh"))
				open_save_popup_ = true;
			ImGui::EndMenu();
		}
	}

	//void popups() override
	//{
	//	if (open_save_popup_)
	//		ImGui::OpenPopup("Save");

	//	if (ImGui::BeginPopupModal("Save", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	//	{
	//		static MESH* selected_mesh = nullptr;
	//		static char filename[32] = "\0";
	//		static std::string filetype = (*supported_formats_)[0];
	//		static std::function<void()> cleanup = []() {};
	//		bool close_popup = false;

	//		imgui_mesh_selector(this, selected_mesh, "Mesh", [&](MESH& m) { selected_mesh = &m; });
	//		if (ImGui::BeginCombo("Filetype", filetype.c_str()))
	//		{
	//			for (const std::string& t : *supported_formats_)
	//			{
	//				bool is_selected = t == filetype;
	//				if (ImGui::Selectable(t.c_str(), is_selected))
	//					filetype = t;
	//				if (is_selected)
	//					ImGui::SetItemDefaultFocus();
	//			}
	//			ImGui::EndCombo();
	//		}
	//		ImGui::InputText("Filename", filename, 32);

	//		if (selected_mesh)
	//		{
	//			static std::shared_ptr<Attribute<Vec3>> selected_vertex_position = nullptr;
	//			imgui_combo_attribute<Vertex, Vec3>(
	//				*selected_mesh, selected_vertex_position, "Position",
	//				[&](const std::shared_ptr<Attribute<Vec3>>& attribute) { selected_vertex_position = attribute; });
	//			if (selected_vertex_position)
	//			{
	//				if (ImGui::Button("Save", ImVec2(120, 0)))
	//				{
	//					if constexpr (mesh_traits<MESH>::dimension == 1)
	//						save_graph_to_file(*selected_mesh, selected_vertex_position.get(), filetype, filename);
	//					if constexpr (mesh_traits<MESH>::dimension == 2)
	//						save_surface_to_file(*selected_mesh, selected_vertex_position.get(), filetype, filename);
	//					if constexpr (mesh_traits<MESH>::dimension == 3)
	//						save_volume_to_file(*selected_mesh, selected_vertex_position.get(), filetype, filename);
	//					close_popup = true;
	//				}
	//			}
	//			cleanup = [&]() { selected_vertex_position = nullptr; };
	//		}

	//		if (ImGui::Button("Cancel", ImVec2(120, 0)))
	//			close_popup = true;

	//		if (close_popup)
	//		{
	//			selected_mesh = nullptr;
	//			filename[0] = '\0';
	//			cleanup();
	//			ImGui::CloseCurrentPopup();
	//		}
	//		ImGui::EndPopup();
	//	}
	//}

	void left_panel() override
	{
		imgui_mesh_selector(this, selected_mesh_, "Mesh", [&](MESH& m) {
			selected_mesh_ = &m;
			mesh_data(m).outlined_until_ = App::frame_time_ + 1.0;
		});

		if (selected_mesh_)
		{
			MeshData<MESH>& md = mesh_data(*selected_mesh_);

			imgui_combo_attribute<Vertex, Vec3>(*selected_mesh_, md.bb_vertex_position_, "Position",
												[&](const std::shared_ptr<Attribute<Vec3>>& attribute) {
													set_mesh_bb_vertex_position(*selected_mesh_, attribute);
												});

			if (ImGui::Button("Clone"))
				clone_mesh(*selected_mesh_);

			ImGui::Separator();
			ImGui::TextUnformatted("Size");
			ImGui::Separator();

			if (ImGui::BeginTable("MeshSize", 2))
			{
				ImGui::TableSetupColumn("CellType");
				ImGui::TableSetupColumn("Number");
				ImGui::TableHeadersRow();

				for (uint32 i = 0; i < std::tuple_size<typename mesh_traits<MESH>::Cells>::value; ++i)
				{
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(mesh_traits<MESH>::cell_names[i]);
					ImGui::TableNextColumn();
					ImGui::Text("%d", md.nb_cells_[i]);
				}
				ImGui::EndTable();
			}

			ImGui::Separator();
			ImGui::TextUnformatted("Attributes");
			ImGui::Separator();

			if (ImGui::BeginTable("MeshAttributes", 2))
			{
				ImGui::TableSetupColumn("CellType");
				ImGui::TableSetupColumn("Name");
				ImGui::TableHeadersRow();

				auto names = md.attributes_names();
				for (uint32 i = 0; i < std::tuple_size<typename mesh_traits<MESH>::Cells>::value; ++i)
				{
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(mesh_traits<MESH>::cell_names[i]);
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-1);
					if (ImGui::ListBoxHeader((std::string("##") + mesh_traits<MESH>::cell_names[i]).c_str(),
											 names[i].size()))
					{
						for (auto& n : names[i])
							ImGui::Text("%s", n.c_str());
						ImGui::ListBoxFooter();
					}
					// ImGui::PopItemWidth();
					// ImGui::NextColumn();
					// ImGui::NextColumn();
					// ImGui::PushItemWidth(-1);
					// ImGui::InputText((std::string("##") + mesh_traits<MESH>::cell_names[i]).c_str(),
					// new_attribute_name_[i], 				 32); ImGui::PopItemWidth(); ImGui::SameLine(); if
					// (ImGui::Button((std::string("Add##") + mesh_traits<MESH>::cell_names[i]).c_str()))
					// {
					// }
				}
				ImGui::EndTable();
			}
		}
	}

private:
	std::vector<std::string> supported_surface_formats_ = {"obj"};
	std::vector<std::string> supported_surface_files_ = {"Surface", "*.obj"};
	std::vector<std::string>* supported_formats_ = nullptr;

	bool open_save_popup_ = false;

	const MeshObjTriplet* selected_mesh_;
	// std::array<char[32], std::tuple_size<typename mesh_traits<MESH>::Cells>::value> new_attribute_name_;

	std::unordered_map<std::string, std::unique_ptr<MeshObjTriplet>> meshes_;
	std::unordered_map<const MeshObjTriplet*, std::string> mesh_filename_;
	std::unordered_map<const CMap2*, MeshData<CMap2> mesh_data_;
	Vec3 bb_min_, bb_max_;
};











class SurfaceObjRender : public ViewModule
{

	template <>
	struct mesh_traits < MeshObjTriplet >
	{
		static constexpr const char* name = "MeshObjTriplet";
		static constexpr const uint8 dimension = 2;

		using Parent = CMap2::Parent;

		using Vertex = CMap2::Vertex;
		using HalfEdge = CMap2::HalfEdge;
		using Edge = CMap2::Edge;
		using Face = CMap2::Face;
		using Volume = CMap2::Volume;

		using Cells = std::tuple<Vertex, HalfEdge, Edge, Face, Volume>;
		static constexpr const char* cell_names[] = {"Vertex", "HalfEdge", "Edge", "Face", "Volume"};

		template <typename T>
		using Attribute = CMapBase::Attribute<T>;
		using AttributeGen = CMapBase::AttributeGen;
		using MarkAttribute = CMapBase::MarkAttribute;
	};


	static_assert(mesh_traits<struct MeshObjTriplet>::dimension >= 2, "SurfaceRender can only be used with meshes of dimension >= 2");


	template <typename T>
	using Attribute = typename mesh_traits<struct MeshObjTriplet>::template Attribute<T>;

	using Vertex = typename mesh_traits<struct MeshObjTriplet>::Vertex;
	using Edge = typename mesh_traits<struct MeshObjTriplet>::Edge;
	using Face = typename mesh_traits<struct MeshObjTriplet>::Face;

	struct Parameters
	{
		Parameters()
			: vertex_position_(nullptr), vertex_position_vbo_(nullptr), vertex_tc_(nullptr), vertex_tc_vbo_(nullptr),
			  draw_flatten_(false), draw_bound_(BoundaryNONE)
		{
			param_textured_ = rendering::ShaderObjFlatTexture::generate_param();
			param_flatten_ = rendering::ShaderObjMeshUV::generate_param();
			param_boundary_edges_ = rendering::ShaderMesh2DEdges::generate_param();
		}

		CGOGN_NOT_COPYABLE_NOR_MOVABLE(Parameters);

		std::unique_ptr<rendering::ShaderObjMeshUV::Param> param_flatten_;
		std::unique_ptr<rendering::ShaderObjFlatTexture::Param> param_textured_;
		std::unique_ptr<rendering::ShaderMesh2DEdges::Param> param_boundary_edges_;		

		std::shared_ptr<Attribute<Vec3>> vertex_position_;
		rendering::VBO* vertex_position_vbo_;

		std::shared_ptr<Attribute<Vec2>> vertex_tc_;
		rendering::VBO* vertex_tc_vbo_;

		std::array<rendering::EBO, 2> tri_ebos_;
		bool draw_flatten_;
		std::array<rendering::EBO, 3> boundary_ebos_;
		OBJBoundaries draw_bound_;
	};

public:
	SurfaceObjRender(const App& app)
		: ViewModule(app, "SurfaceRender (" + std::string{mesh_traits<struct MeshObjTriplet>::name} + ")"),
		  selected_view_(app.current_view()), selected_mesh_pos_(nullptr)
	{
	}

	~SurfaceObjRender()
	{
	}

private:

	void update_ebo(Parameters& p, const struct MeshObjTriplet* tm)
	{
		CMap2& map_pos = tm->map_pos;
		CMap2& map_tc = tm->map_tc;
		CMap2& map_no = tm->map_no;

		using Vertex = typename mesh_traits<struct MeshObjTriplet>::Vertex;
		using Face = typename mesh_traits<struct MeshObjTriplet>::Face;
		using Edge = typename mesh_traits<struct MeshObjTriplet>::Edge;

		std::vector<uint32> table_pos_indices;
		table_pos_indices.reserve(8192);
		std::vector<uint32> table_tc_indices;
		table_tc_indices.reserve(8192);
		//std::vector<uint32> table_no_indices;
		//table_no_indices.reserve(8192);

		std::vector<Vertex> vertices;
		vertices.reserve(32u);

		foreach_cell(map_pos, [&](Face f) -> bool
		{
			vertices.clear();
			append_incident_vertices(map_pos, f, vertices);
			for (uint32 i = 1; i < uint32(vertices.size()) - 1; ++i)
			{
				table_pos_indices.push_back(index_of(map_pos, vertices[0]));
				table_pos_indices.push_back(index_of(map_pos, vertices[i]));
				table_pos_indices.push_back(index_of(map_pos, vertices[i + 1]));
				table_tc_indices.push_back(index_of(map_tc, vertices[0]));
				table_tc_indices.push_back(index_of(map_tc, vertices[i]));
				table_tc_indices.push_back(index_of(map_tc, vertices[i + 1]));
				//table_no_indices.push_back(index_of(map_no, vertices[0]));
				//table_no_indices.push_back(index_of(map_no, vertices[i]));
				//table_no_indices.push_back(index_of(map_no, vertices[i + 1]));
			}
			return true;
		});

		if (!p.tri_ebos_[0].is_created())
			p.tri_ebos_[0].create();
		p.tri_ebos_[0].bind();
		p.tri_ebos_[0].allocate(table_pos_indices.size());
		uint32* ptr =p.tri_ebos_[0].lock_pointer();
		std::memcpy(ptr, table_pos_indices.data(), sizeof(uint32) * table_pos_indices.size());
		p.tri_ebos_[0].set_name("EBO_Pos");
		p.tri_ebos_[0].release_pointer();
		p.tri_ebos_[0].release();

		if (!p.tri_ebos_[1].is_created())
			p.tri_ebos_[1].create();
		p.tri_ebos_[1].bind();
		p.tri_ebos_[1].allocate(table_pos_indices.size());
		ptr = p.tri_ebos_[1].lock_pointer();
		std::memcpy(ptr, table_tc_indices.data(), sizeof(uint32) * table_pos_indices.size());
		p.tri_ebos_[1].set_name("EBO_TC");
		p.tri_ebos_[1].release_pointer();
		p.tri_ebos_[1].release();

		//if (!p.tri_ebos_[2].is_created())
		//	p.tri_ebos_[2].create();
		//p.tri_ebos_[2].bind();
		//p.tri_ebos_[2].allocate(table_pos_indices.size());
		//ptr = p.tri_ebos_[2].lock_pointer();
		//std::memcpy(ptr, table_no_indices.data(), sizeof(uint32) * table_pos_indices.size());
		//p.tri_ebos_[2].set_name("EBO_No");
		//p.tri_ebos_[2].release_pointer();
		//p.tri_ebos_[2].release();


		std::vector<uint32> table_pos_boundary_indices;
		table_pos_boundary_indices.reserve(8192);

		foreach_cell(map_pos, [&](Edge e) -> bool {
			if (is_boundary(map_pos, e.dart_) || is_boundary(map_pos, phi2(map_pos, e.dart_)))
			{
				table_pos_boundary_indices.push_back(index_of(map_tc, Vertex(e.dart_)));
				table_pos_boundary_indices.push_back(index_of(map_tc, Vertex(phi1(map_pos, e.dart_))));
			}
			return true;
		});

		if (!p.boundary_ebos_[0].is_created())
			p.boundary_ebos_[0].create();
		p.boundary_ebos_[0].bind();
		p.boundary_ebos_[0].allocate(table_pos_boundary_indices.size());
		ptr = p.boundary_ebos_[0].lock_pointer();
		std::memcpy(ptr, table_pos_boundary_indices.data(), sizeof(uint32) * table_pos_boundary_indices.size());
		p.boundary_ebos_[0].set_name("EBO_Bound_pos");
		p.boundary_ebos_[0].release_pointer();
		p.boundary_ebos_[0].release();


		std::vector<uint32> table_tc_boundary_indices;
		table_tc_boundary_indices.reserve(8192);

		foreach_cell(map_tc, [&](Edge e) -> bool {
			if (is_boundary(map_tc, e.dart_) || is_boundary(map_tc, phi2(map_tc, e.dart_)))
			{
				table_tc_boundary_indices.push_back(index_of(map_tc, Vertex(e.dart_)));
				table_tc_boundary_indices.push_back(index_of(map_tc, Vertex(phi1(map_tc, e.dart_))));
			}
			return true;
		});

		if (!p.boundary_ebos_[1].is_created())
			p.boundary_ebos_[1].create();
		p.boundary_ebos_[1].bind();
		p.boundary_ebos_[1].allocate(table_tc_boundary_indices.size());
		ptr = p.boundary_ebos_[1].lock_pointer();
		std::memcpy(ptr, table_tc_boundary_indices.data(), sizeof(uint32) * table_tc_boundary_indices.size());
		p.boundary_ebos_[1].set_name("EBO_Bound_TC");
		p.boundary_ebos_[1].release_pointer();
		p.boundary_ebos_[1].release();


		// std::vector<uint32> table_no_boundary_indices;
		// table_no_boundary_indices.reserve(8192);

		// foreach_cell(map_no, [&](Edge e) -> bool {
		//	if (is_boundary(map_no, e.dart_) || is_boundary(map_no, phi2(map_no, e.dart_)))
		//	{
		//		table_no_boundary_indices.push_back(index_of(map_tc, Vertex(e.dart_)));
		//		table_no_boundary_indices.push_back(index_of(map_tc, Vertex(phi1(map_no, e.dart_))));
		//	}
		//	return true;
		//});

		//if (!p.boundary_ebos_[2].is_created())
		//	p.boundary_ebos_[2].create();
		//p.boundary_ebos_[2].bind();
		//p.boundary_ebos_[2].allocate(table_no_boundary_indices.size());
		//ptr = p.boundary_ebos_[2].lock_pointer();
		//std::memcpy(ptr, table_no_boundary_indices.data(), sizeof(uint32) * table_no_boundary_indices.size());
		//p.boundary_ebos_[2].set_name("EBO_Bound_No");
		//p.boundary_ebos_[2].release_pointer();
		//p.boundary_ebos_[2].release();
	}

	void init_mesh(MeshObjTriplet* mot)
	{
		const std::string& bname = mesh_provider_->mesh_name(*mot);

		for (View* v : linked_views_)
		{
			Parameters& p = parameters_[v][mot];
			update_ebo(p, mot);
			std::shared_ptr<Attribute<Vec3>> vertex_pos = cgogn::get_attribute<Vec3, Vertex>(mot->map_pos, "position");
			std::shared_ptr<Attribute<Vec3>> vertex_tc = cgogn::get_attribute<Vec2, Vertex>(mot->map_tc, "position");
//			std::shared_ptr<Attribute<Vec3>> vertex_no = cgogn::get_attribute<Vec3, Vertex>(mot->map_no, "position");

			if (vertex_pos || vertex_tc)
				set_vertex_position(*v, *mot, vertex_pos,vertex_tc);
		}
	}



public:
	void load_texture(const std::string& img_name)
	{
		rendering::GLImage img(img_name);
		tex_->load(img);
	}

	void load_texture(const rendering::GLImage& img)
	{
		tex_->load(img);
	}

	void set_vertex_attributes(View& v, const MeshObjTriplet& mot, const std::shared_ptr<Attribute<Vec3>>& vertex_pos,
							  const std::shared_ptr<Attribute<Vec3>>& vertex_tc,
							  /* const std::shared_ptr<Attribute<Vec3>>& vertex_no*/)
	{
		CMap2& map_pos = mot->map_pos;
		CMap2& map_tc = mot->map_tc;
		//CMap2& map_no = mot->map_no;

		Parameters& p = parameters_[&v][&m];
		if (p.vertex_position_ != vertex_pos)
		{
			p.vertex_position_ = vertex_position;
			if (p.vertex_position_)
				p.vertex_position_vbo_ = mesh_provider_->mesh_data(map_pos).update_vbo(p.vertex_position_.get(), true);
			else
				p.vertex_position_vbo_ = nullptr;
		}

		if (p.vertex_tc_ != vertex_tc)
		{
			p.vertex_tc_ = vertex_tc;
			if (p.vertex_tc_)
				p.vertex_tc_vbo_ = mesh_provider_->mesh_data(map_tc).update_vbo(p.vertex_tc_.get(), true);
			else
				p.vertex_tc_vbo_ = nullptr;
		}

		//if (p.vertex_no_ != vertex_no)
		//{
		//	p.vertex_no_ = vertex_no;
		//	if (p.vertex_no_)
		//		p.vertex_no_vbo_ = mesh_provider_->mesh_data(map_no).update_vbo(p.vertex_no_.get(), true);
		//	else
		//		p.vertex_no_vbo_ = nullptr;
		//}

		p.param_textured_->set_vbos({p.vertex_position_vbo_, p.vertex_tc_vbo_});
		p.param_flatten_->set_vbos({p.vertex_tc_vbo_});
		p.param_boundary_edges_->set_vbos({p.vertex_tc_vbo_});
		v.request_update();	
	}


	void init() override
	{
		mesh_provider_ = static_cast<ui::MeshProvider<MeshObjTriplet>*>(
			app_.module("MeshProvider (" + std::string{mesh_traits<MeshObjTriplet>::name} + ")"));
		mesh_provider_->foreach_mesh([this](MeshObjTriplet& m, const std::string&) { init_mesh(&m); });
		connections_.push_back(boost::synapse::connect<typename MeshProvider<MeshObjTriplet>::mesh_added>(
			mesh_provider_, this, &SurfaceObjRender<MeshObjTriplet>::init_mesh));

		std::vector<std::pair<GLenum, GLint>> param_texture {
			{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
			{GL_TEXTURE_MAG_FILTER, GL_NEAREST},
			{GL_TEXTURE_WRAP_S, GL_REPEAT},
			{GL_TEXTURE_WRAP_T, GL_REPEAT}}

		tex_ = std::make_shared<rendering::Texture2D>(param_texture);
	}

	void draw(View* view) override
	{
		for (auto& [m, pp] : parameters_[view])
		{
			Parameters& p = pp;

			const rendering::GLMat4& proj_matrix = view->projection_matrix();
			const rendering::GLMat4& view_matrix = view->modelview_matrix();

			if (p.draw_flatten_)
			{
				rendering::GLVec2 ratio = (view->viewport_width() > view->viewport_height())
						? rendering::GLVec2(float32(view->viewport_height()) / view->viewport_width(), 1.0f)
						: rendering::GLVec2(1.0f, view->viewport_width() / float32(view->viewport_height()));
				ratio *= 0.95f;

				if (p.param_flatten_->attributes_initialized())
				{
					p.param_flatten_->ratio_ = ratio;
					glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
					p.tri_ebos_[1].bind_texture_buffer(10);
					p.param_flatten_->bind();
					glDrawArrays(GL_TRIANGLES, 0, p.tri_ebos_[0].size());
					p.param_flatten_->release();
					p.tri_ebos_[1].release_texture_buffer(10);
					glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
				}

				if (p.draw_bound_ != BoundaryNONE)
				{
					if ((p.param_boundary_edges_->attributes_initialized()))
					{
						glDisable(GL_DEPTH_TEST);
						auto& ebo = p.boundary_ebos_[p.draw_bound_];
						ebo.bind_texture_buffer(10);
						p.param_boundary_edges_->ratio_ = ratio;
						p.param_boundary_edges_->color_ = rendering::GLColor(0, 1, 1, 1);
						p.param_boundary_edges_->bind();
						glDrawArrays(GL_LINES, 0, ebo.size());
						p.param_boundary_edges_->release();
						ebo.release_texture_buffer(10);
						glEnable(GL_DEPTH_TEST);
					}
				}
			}
			else
			{
				if (p.param_textured_->attributes_initialized())
				{
					p.param_textured_->texture_ = tex_;
					p.tri_ebos_[0].bind_texture_buffer(10);
					p.tri_ebos_[1].bind_texture_buffer(11);
					p.param_textured_->bind(proj_matrix, view_matrix);
					glDrawArrays(GL_TRIANGLES, 0, p.tri_ebos_[0].size());
					p.param_textured_->release();
					p.tri_ebos_[1].release_texture_buffer(11);
					p.tri_ebos_[0].release_texture_buffer(10);
				}
			}
		}
	}

	void left_panel() override
	{
		bool need_update = false;

		if (app_.nb_views() > 1)
			imgui_view_selector(this, selected_view_, [&](View* v) { selected_view_ = v; });

		imgui_mesh_selector(mesh_provider_, selected_mesh_, "Surf Obj Position", [&](MeshObjTriplet& m) {
			selected_mesh_ = &m;
			const std::string& bname = mesh_provider_->mesh_name(m);
			std::string tc_name = bname + "_tc";
//			std::string no_name = bname + "_no";
			mesh_provider_->foreach_mesh([&](MeshObjTriplet& mm, const std::string& name) {
				if (name == tc_name)
					selected_mesh_tc_ = &mm;
//				if (name == no_name)
//					selected_mesh_no_ = &mm;
			});

		});

		if (selected_view_ && selected_mesh_pos_ && selected_mesh_tc_ /* && selected_mesh_no_*/)
		{
			Parameters& p = parameters_[selected_view_][selected_mesh_pos_];
			if (ImGui::Checkbox("draw_flatten", &p.draw_flatten_))
				need_update = true;
			if (p.draw_flatten_)
			{
				ImGui::Separator();
				ImGui::TextUnformatted("draw boundary");
				ImGui::BeginGroup();
				int* dr_bound = reinterpret_cast<int*>(& p.draw_bound_);
				need_update |= ImGui::RadioButton("Pos##bound", dr_bound, BoundaryPos);
				ImGui::SameLine();
				need_update |= ImGui::RadioButton("TC##bound", dr_bound, BoundaryTC);
				ImGui::SameLine();
				need_update |= ImGui::RadioButton("NONE##bound", dr_bound, BoundaryNONE);
				ImGui::EndGroup();

			}
			else
			{
				if (ImGui::Checkbox("draw_param", &p.param_textured_->draw_param_))
								need_update = true;
			}

			if (need_update)
				for (View* v : linked_views_)
					v->request_update();
		}
	}

private:
	View* selected_view_;
	const MeshObjTriplet* selected_mesh_;
	std::unordered_map<View*, std::unordered_map<const MeshObjTriplet*, Parameters>> parameters_;
	std::vector<std::shared_ptr<boost::synapse::connection>> connections_;
	std::unordered_map<const MeshObjTriplet*, std::vector<std::shared_ptr<boost::synapse::connection>>> mesh_connections_;
	MeshProvider<MeshObjTriplet>* mesh_provider_;
	std::shared_ptr<rendering::Texture2D> tex_;
};

} // namespace ui

} // namespace cgogn

#endif // CGOGN_MODULE_SURFACE_RENDER_H_
