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

#ifndef CGOGN_IO_SURFACE_IMPORT_H_
#define CGOGN_IO_SURFACE_IMPORT_H_

#include <cgogn/io/cgogn_io_export.h>

#include <cgogn/core/types/map/cmap/cmap2.h>
#include <cgogn/core/types/incidence_graph/incidence_graph.h>

#include <cgogn/geometry/types/vector_traits.h>

#include <vector>

namespace cgogn
{

namespace io
{

using Vec3 = geometry::Vec3;

struct SurfaceImportData
{
	uint32 nb_vertices_ = 0;
	uint32 nb_faces_ = 0;

	std::vector<Vec3> vertex_position_;
	std::string vertex_position_attribute_name_ = "position";

	std::vector<uint32> faces_nb_vertices_;
	std::vector<uint32> faces_vertex_indices_;

	std::vector<uint32> vertex_id_after_import_;

	inline void reserve(uint32 nb_vertices, uint32 nb_faces)
	{
		nb_vertices_ = nb_vertices;
		nb_faces_ = nb_faces;
		vertex_position_.reserve(nb_vertices);
		faces_nb_vertices_.reserve(nb_faces);
		faces_vertex_indices_.reserve(nb_faces * 4u);
		vertex_id_after_import_.reserve(nb_vertices);
	}
};

void CGOGN_IO_EXPORT import_surface_data(IncidenceGraph& m, SurfaceImportData& surface_data);

#ifndef _SURFACE_IMPORT_CPP_
template <typename MAP>
auto import_surface_data(MAP& m, SurfaceImportData& surface_data)
	-> std::enable_if_t<std::is_convertible_v<MAP&, struct CMap2&> || std::is_convertible_v<MAP&, struct GMap2&>, void>
{
	using Vertex = typename mesh_traits<MAP>::Vertex;

	auto position = get_or_add_attribute<geometry::Vec3, Vertex>(m, surface_data.vertex_position_attribute_name_);

	for (uint32 i = 0u; i < surface_data.nb_vertices_; ++i)
	{
		uint32 vertex_id = new_index<Vertex>(m);
		(*position)[vertex_id] = surface_data.vertex_position_[i];
		surface_data.vertex_id_after_import_.push_back(vertex_id);
	}

	auto darts_per_vertex = add_attribute<std::vector<Dart>, Vertex>(m, "__darts_per_vertex");

	uint32 faces_vertex_index = 0u;
	std::vector<uint32> vertices_buffer;
	vertices_buffer.reserve(16u);

	for (uint32 i = 0u; i < surface_data.nb_faces_; ++i)
	{
		uint32 nbv = surface_data.faces_nb_vertices_[i];

		vertices_buffer.clear();
		uint32 prev = std::numeric_limits<uint32>::max();

		for (uint32 j = 0u; j < nbv; ++j)
		{
			uint32 idx = surface_data.vertex_id_after_import_[surface_data.faces_vertex_indices_[faces_vertex_index++]];
			if (idx != prev)
			{
				prev = idx;
				vertices_buffer.push_back(idx);
			}
		}
		if (vertices_buffer.front() == vertices_buffer.back())
			vertices_buffer.pop_back();

		nbv = uint32(vertices_buffer.size());
		if (nbv > 2u)
		{
			using MAP1 = typename mesh_traits<MAP>::ParentDimType;
			typename MAP1::Face f = add_face(static_cast<MAP1&>(m), nbv, false);
			Dart d = f.dart;
			for (uint32 j = 0u; j < nbv; ++j)
			{
				const uint32 vertex_index = vertices_buffer[j];
				set_index<Vertex>(m, d, vertex_index);
				if constexpr (std::is_convertible_v<MAP&, struct GMap2&>)
				{
					set_index<Vertex>(m, beta1(m, d), vertex_index);
				}
				(*darts_per_vertex)[vertex_index].push_back(d);
				d = phi1(m, d);
			}
		}
	}

	bool need_vertex_unicity_check = false;
	uint32 nb_boundary_edges = 0u;

	for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
	{
		if (phi2(m, d) == d)
		{
			uint32 vertex_index = index_of(m, Vertex(d));

			const std::vector<Dart>& next_vertex_darts =
				value<std::vector<Dart>>(m, darts_per_vertex, Vertex(phi1(m, d)));
			bool phi2_found = false;
			bool first_OK = true;

			for (auto it = next_vertex_darts.begin(); it != next_vertex_darts.end() && !phi2_found; ++it)
			{
				if (index_of(m, Vertex(phi1(m, *it))) == vertex_index)
				{
					if (phi2(m, *it) == *it)
					{
						phi2_sew(m, d, *it);
						phi2_found = true;
					}
					else
						first_OK = false;
				}
			}

			if (!phi2_found)
				++nb_boundary_edges;

			if (!first_OK)
				need_vertex_unicity_check = true;
		}
	}

	if constexpr (std::is_same_v<MAP, struct CMap2>)
	{
		if (nb_boundary_edges > 0u)
		{
			uint32 nb_holes = close(m);
			std::cout << nb_holes << " hole(s) have been closed" << std::endl;
			std::cout << nb_boundary_edges << " boundary edges" << std::endl;
		}
	}
	   // if (need_vertex_unicity_check)
	   // {
	   // 	map_.template enforce_unique_orbit_embedding<Vertex::ORBIT>();
	   // 	cgogn_log_warning("create_map") << "Import Surface: non manifold vertices detected and corrected";
	   // }

	remove_attribute<Vertex>(m, darts_per_vertex);
}
#endif

} // namespace io

} // namespace cgogn

#endif // CGOGN_IO_SURFACE_IMPORT_H_
