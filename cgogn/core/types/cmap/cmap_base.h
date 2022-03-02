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

#ifndef CGOGN_CORE_CMAP_CMAP_BASE_H_
#define CGOGN_CORE_CMAP_CMAP_BASE_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/container/attribute_container.h>
#include <cgogn/core/types/container/chunk_array.h>
#include <cgogn/core/types/container/vector.h>

#include <cgogn/core/types/cmap/cell.h>
#include <cgogn/core/utils/tuples.h>
#include <cgogn/core/types/mesh_traits.h>
#include <any>
#include <array>
#include <unordered_map>
#include <sstream>


namespace cgogn
{


struct CGOGN_CORE_EXPORT CMapBase
{

    // using AttributeContainer = AttributeContainerT<Vector>;
	using AttributeContainer = AttributeContainerT<ChunkArray>;

	template <typename T>
	using Attribute = AttributeContainer::Attribute<T>;
	using AttributeGen = AttributeContainer::AttributeGen;
	using MarkAttribute = AttributeContainer::MarkAttribute;

	/*************************************************************************/
	// Map-wise attributes container
	/*************************************************************************/
	std::unordered_map<std::string, std::any> attributes_;

	/*************************************************************************/
	// Dart attributes container
	/*************************************************************************/
	mutable AttributeContainer darts_;

	// shortcuts to topological relations attributes
	std::vector<std::shared_ptr<Attribute<Dart>>> relations_;
	// shortcuts to cells indices attributes
	std::array<std::shared_ptr<Attribute<uint32>>, NB_ORBITS> cells_indices_;

	// shortcut to boundary marker attribute
	MarkAttribute* boundary_marker_;

	/*************************************************************************/
	// Cells attributes containers
	/*************************************************************************/
	mutable std::array<AttributeContainer, NB_ORBITS> attribute_containers_;

	enum TraversalPolicy
	{
		AUTO,
		DART_MARKING
	};

	CMapBase();
	~CMapBase();

	// Map-wise attributes
	template <typename T>
	T& get_attribute(const std::string& name)
	{
		auto [it, inserted] = attributes_.try_emplace(name, T());
		return std::any_cast<T&>(it->second);
	}

	inline std::shared_ptr<Attribute<Dart>> add_relation(const std::string& name)
	{
		return relations_.emplace_back(darts_.add_attribute<Dart>(name));
	}

	inline Dart begin() const
	{
		return Dart(darts_.first_index());
	}
	inline Dart end() const
	{
		return Dart(darts_.last_index());
	}
	inline Dart next(Dart d) const
	{
		return Dart(darts_.next_index(d.index));
	}
};

#include <cgogn/core/types/mesh_traits.h>
//#include <cgogn/core/functions/mesh_info.h>
#include <cgogn/core/functions/traversals/global.h>

template <typename MESH, typename CELL, typename FUNC>
auto foreach_dart_of_orbit(const MESH& m, CELL c, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	static const Orbit orbit = CELL::ORBIT;

	if constexpr (orbit == DART)
	{
		unused_parameters(m);
		f(c.dart);
		return;
	}
	if constexpr (orbit == PHI1)
	{
		foreach_dart_of_PHI1(m, c.dart, f);
		return;
	}
	if constexpr (orbit == PHI2)
	{
		foreach_dart_of_PHI2(m, c.dart, f);
		return;
	}
	if constexpr (orbit == PHI21)
	{
		foreach_dart_of_PHI21(m, c.dart, f);
		return;
	}
	if constexpr (orbit == PHI1_PHI2)
	{
		foreach_dart_of_PHI1_PHI2(m, c.dart, f);
		return;
	}
	if constexpr (orbit == PHI1_PHI3)
	{
		foreach_dart_of_PHI1_PHI3(m, c.dart, f);
		return;
	}
	if constexpr (orbit == PHI2_PHI3)
	{
		foreach_dart_of_PHI2_PHI3(m, c.dart, f);
		return;
	}
	if constexpr (orbit == PHI21_PHI31)
	{
		foreach_dart_of_PHI21_PHI31(m, c.dart, f);
		return;
	}
	if constexpr (orbit == PHI1_PHI2_PHI3)
	{
		foreach_dart_of_PHI1_PHI2_PHI3(m, c.dart, f);
		return;
	}
}

template <typename CELL, typename CMAP>
uint32 nb_darts_of_orbit(const CMAP& m, CELL c)
{
	static_assert(is_in_tuple<CELL, typename CMAP::Cells>::value, "CELL not supported in this CMAP");
	uint32 result = 0;
	foreach_dart_of_orbit(m, c, [&](Dart) -> bool {
		++result;
		return true;
	});
	return result;
}

/*****************************************************************************/
// orbits traversals
/*****************************************************************************/

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI1(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
	Dart it = d;
	do
	{
		if (!f(it))
			break;
		it = phi1(m, it);
	} while (it != d);
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI2(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
	if (f(d))
		f(phi2(m, d));
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI21(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
	Dart it = d;
	do
	{
		if (!f(it))
			break;
		it = phi<-1, 2>(m, it);
	} while (it != d);
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI1_PHI2(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	DartMarkerStore<MESH> marker(m);

	std::vector<Dart> visited_faces;
	visited_faces.push_back(d); // Start with the face of d

	// For every face added to the list
	for (uint32 i = 0; i < uint32(visited_faces.size()); ++i)
	{
		const Dart e = visited_faces[i];
		if (!marker.is_marked(e)) // Face has not been visited yet
		{
			// mark visited darts (current face)
			// and add non visited adjacent faces to the list of face
			Dart it = e;
			do
			{
				if (!f(it)) // apply the function to the darts of the face
					return;
				marker.mark(it);			  // Mark
				const Dart adj = phi2(m, it); // Get adjacent face
				if (!marker.is_marked(adj))
					visited_faces.push_back(adj); // Add it
				it = phi1(m, it);
			} while (it != e);
		}
	}
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI1_PHI3(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
	foreach_dart_of_PHI1(m, d, [&](Dart fd) -> bool {
		if (f(fd))
			return f(phi3(m, fd));
		return false;
	});
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI2_PHI3(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
	Dart it = d;
	do
	{
		if (!f(it))
			break;
		it = phi2(m, it);
		if (!f(it))
			break;
		it = phi3(m, it);
	} while (it != d);
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI21_PHI31(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	DartMarkerStore<MESH> marker(m);
	const std::vector<Dart>& marked_darts = marker.marked_darts();

	marker.mark(d);
	for (uint32 i = 0; i < uint32(marked_darts.size()); ++i)
	{
		const Dart curr_dart = marked_darts[i];
		//			if ( !(is_boundary(curr_dart) && is_boundary(phi3(curr_dart))) )
		if (!f(curr_dart))
			break;

		const Dart d_1 = phi_1(m, curr_dart);
		const Dart d2_1 = phi2(m, d_1); // turn in volume
		const Dart d3_1 = phi3(m, d_1); // change volume

		if (!marker.is_marked(d2_1))
			marker.mark(d2_1);
		if (!marker.is_marked(d3_1))
			marker.mark(d3_1);
	}
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI1_PHI2_PHI3(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	DartMarkerStore<MESH> marker(m);

	std::vector<Dart> visited_face2;
	visited_face2.push_back(d); // Start with the face of d

	// For every face added to the list
	for (uint32 i = 0; i < visited_face2.size(); ++i)
	{
		const Dart e = visited_face2[i];
		if (!marker.is_marked(e)) // Face2 has not been visited yet
		{
			// mark visited darts (current face2)
			// and add non visited phi2-adjacent face2 to the list of face2
			Dart it = e;
			do
			{
				if (!f(it)) // apply the function to the darts of the face2
					return;
				marker.mark(it);			   // Mark
				const Dart adj2 = phi2(m, it); // Get phi2-adjacent face2
				if (!marker.is_marked(adj2))
					visited_face2.push_back(adj2); // Add it
				it = phi1(m, it);
			} while (it != e);
			// add phi3-adjacent face2 to the list
			visited_face2.push_back(phi3(m, it));
		}
	}
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_ALPHA0(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
	if (f(d))
		f(alpha0(m, d));
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_ALPHA1(const MESH& m, Dart d, const FUNC& f)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
	Dart it = d;
	do
	{
		if (!f(it))
			break;
		it = alpha1(m, it);
	} while (it != d);
}



template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_vertex(const MESH& m, CELL c, const FUNC& func)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	foreach_incident_vertex(m, c, func, CMapBase::TraversalPolicy::AUTO);
}

template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_vertex(const MESH& m, CELL c, const FUNC& func, CMapBase::TraversalPolicy traversal_policy)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	using Vertex = typename mesh_traits<MESH>::Vertex;

	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	static_assert(is_func_parameter_same<FUNC, Vertex>::value, "Wrong function cell parameter type");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	if constexpr (std::is_convertible_v<MESH&, Graph&> && mesh_traits<MESH>::dimension == 1 &&
				  std::is_same_v<CELL, typename mesh_traits<MESH>::Edge>)
	{
		foreach_dart_of_orbit(m, c, [&](Dart d) -> bool { return func(Vertex(d)); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap1&> && mesh_traits<MESH>::dimension == 1 &&
					   std::is_same_v<CELL, typename mesh_traits<MESH>::Face>)
	{
		foreach_dart_of_orbit(m, c, [&](Dart d) -> bool { return func(Vertex(d)); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap2&> && mesh_traits<MESH>::dimension == 2 &&
					   (std::is_same_v<CELL, typename mesh_traits<MESH>::Edge> ||
						std::is_same_v<CELL, typename mesh_traits<MESH>::HalfEdge> ||
						std::is_same_v<CELL, typename mesh_traits<MESH>::Face>))
	{
		foreach_dart_of_orbit(m, c, [&](Dart d) -> bool { return func(Vertex(d)); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap3&> && mesh_traits<MESH>::dimension == 3 &&
					   (std::is_same_v<CELL, typename mesh_traits<MESH>::Edge> ||
						std::is_same_v<CELL, typename mesh_traits<MESH>::HalfEdge>))
	{
		foreach_dart_of_orbit(m, typename mesh_traits<MESH>::Edge2(c.dart),
							  [&](Dart d) -> bool { return func(Vertex(d)); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap3&> && mesh_traits<MESH>::dimension == 3 &&
					   std::is_same_v<CELL, typename mesh_traits<MESH>::Face>)
	{
		foreach_dart_of_orbit(m, typename mesh_traits<MESH>::Face2(c.dart),
							  [&](Dart d) -> bool { return func(Vertex(d)); });
	}
	else
	{
		if (traversal_policy == CMapBase::TraversalPolicy::AUTO && is_indexed<Vertex>(m))
		{
			CellMarkerStore<MESH, Vertex> marker(m);
			foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
				Vertex v(d);
				if (!marker.is_marked(v))
				{
					marker.mark(v);
					return func(v);
				}
				return true;
			});
		}
		else
		{
			DartMarkerStore<MESH> marker(m);
			foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
				if (!marker.is_marked(d))
				{
					Vertex v(d);
					foreach_dart_of_orbit(m, v, [&](Dart d) -> bool {
						marker.mark(d);
						return true;
					});
					return func(v);
				}
				return true;
			});
		}
	}
}


template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_edge(const MESH& m, CELL c, const FUNC& func)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	foreach_incident_edge(m, c, func, CMapBase::TraversalPolicy::AUTO);
}

template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_edge(const MESH& m, CELL c, const FUNC& func, CMapBase::TraversalPolicy traversal_policy)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	using Edge = typename mesh_traits<MESH>::Edge;

	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	static_assert(is_func_parameter_same<FUNC, Edge>::value, "Wrong function cell parameter type");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	if constexpr (std::is_convertible_v<MESH&, Graph&> && mesh_traits<MESH>::dimension == 1 &&
				  std::is_same_v<CELL, typename mesh_traits<MESH>::Vertex>)
	{
		foreach_dart_of_orbit(m, c, [&](Dart d) -> bool { return func(Edge(d)); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap1&> && mesh_traits<MESH>::dimension == 1 &&
					   std::is_same_v<CELL, typename mesh_traits<MESH>::Face>)
	{
		foreach_dart_of_orbit(m, c, [&](Dart d) -> bool { return func(Edge(d)); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap2&> && mesh_traits<MESH>::dimension == 2 &&
					   (std::is_same_v<CELL, typename mesh_traits<MESH>::Vertex> ||
						std::is_same_v<CELL, typename mesh_traits<MESH>::HalfEdge> ||
						std::is_same_v<CELL, typename mesh_traits<MESH>::Face>))
	{
		foreach_dart_of_orbit(m, c, [&](Dart d) -> bool { return func(Edge(d)); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap3&> && mesh_traits<MESH>::dimension == 3 &&
					   std::is_same_v<CELL, typename mesh_traits<MESH>::Face>)
	{
		foreach_dart_of_orbit(m, typename mesh_traits<MESH>::Face2(c.dart),
							  [&](Dart d) -> bool { return func(Edge(d)); });
	}
	else
	{
		if (traversal_policy == CMapBase::TraversalPolicy::AUTO && is_indexed<Edge>(m))
		{
			CellMarkerStore<MESH, Edge> marker(m);
			foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
				Edge e(d);
				if (!marker.is_marked(e))
				{
					marker.mark(e);
					return func(e);
				}
				return true;
			});
		}
		else
		{
			DartMarkerStore<MESH> marker(m);
			foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
				if (!marker.is_marked(d))
				{
					Edge e(d);
					foreach_dart_of_orbit(m, e, [&](Dart d) -> bool {
						marker.mark(d);
						return true;
					});
					return func(e);
				}
				return true;
			});
		}
	}
}


template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_face(const MESH& m, CELL c, const FUNC& func)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	foreach_incident_face(m, c, func, CMapBase::TraversalPolicy::AUTO);
}

template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_face(const MESH& m, CELL c, const FUNC& func, CMapBase::TraversalPolicy traversal_policy)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	using Face = typename mesh_traits<MESH>::Face;

	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	static_assert(is_func_parameter_same<FUNC, Face>::value, "Wrong function cell parameter type");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	if constexpr (std::is_convertible_v<MESH&, CMap2&> && mesh_traits<MESH>::dimension == 2 &&
				  (std::is_same_v<CELL, typename mesh_traits<MESH>::Vertex> ||
				   std::is_same_v<CELL, typename mesh_traits<MESH>::HalfEdge> ||
				   std::is_same_v<CELL, typename mesh_traits<MESH>::Edge>))
	{
		foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
			if (!is_boundary(m, d))
				return func(Face(d));
			return true;
		});
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap3&> && mesh_traits<MESH>::dimension == 3 &&
					   std::is_same_v<CELL, typename mesh_traits<MESH>::Edge>)
	{
		Dart d = c.dart;
		do
		{
			if (!func(Face(d)))
				break;
			d = phi3(m, phi2(m, d));
		} while (d != c.dart);
	}
	else
	{
		if (traversal_policy == CMapBase::TraversalPolicy::AUTO && is_indexed<Face>(m))
		{
			CellMarkerStore<MESH, Face> marker(m);
			foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
				Face f(d);
				if constexpr (mesh_traits<MESH>::dimension == 2) // faces can be boundary cells
				{
					if (!marker.is_marked(f) && !is_boundary(m, d))
					{
						marker.mark(f);
						return func(f);
					}
				}
				else
				{
					if (!marker.is_marked(f))
					{
						marker.mark(f);
						return func(f);
					}
				}
				return true;
			});
		}
		else
		{
			DartMarkerStore<MESH> marker(m);
			foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
				if constexpr (mesh_traits<MESH>::dimension == 2) // faces can be boundary cells
				{
					if (!is_boundary(m, d) && !marker.is_marked(d))
					{
						Face f(d);
						foreach_dart_of_orbit(m, f, [&](Dart d) -> bool {
							marker.mark(d);
							return true;
						});
						return func(f);
					}
				}
				else
				{
					if (!marker.is_marked(d))
					{
						Face f(d);
						foreach_dart_of_orbit(m, f, [&](Dart d) -> bool {
							marker.mark(d);
							return true;
						});
						return func(f);
					}
				}
				return true;
			});
		}
	}
}


template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_volume(const MESH& m, CELL c, const FUNC& func)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	foreach_incident_volume(m, c, func, CMapBase::TraversalPolicy::AUTO);
}

template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_volume(const MESH& m, CELL c, const FUNC& func, CMapBase::TraversalPolicy traversal_policy)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	using Volume = typename mesh_traits<MESH>::Volume;

	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	static_assert(is_func_parameter_same<FUNC, Volume>::value, "Wrong function cell parameter type");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	if constexpr (std::is_convertible_v<MESH&, CMap2&> && mesh_traits<MESH>::dimension == 2)
	{
		func(Volume(c.dart));
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap3&> && mesh_traits<MESH>::dimension == 3 &&
					   std::is_same_v<CELL, typename mesh_traits<MESH>::Edge>)
	{
		Dart d = c.dart;
		do
		{
			if (!is_boundary(m, d))
			{
				if (!func(Volume(d)))
					break;
			}
			d = phi3(m, phi2(m, d));
		} while (d != c.dart);
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap3&> && mesh_traits<MESH>::dimension == 3 &&
					   std::is_same_v<CELL, typename mesh_traits<MESH>::Face>)
	{
		Dart d = c.dart;
		if (!is_boundary(m, d))
			if (!func(Volume(d)))
				return;
		d = phi3(m, d);
		if (!is_boundary(m, d))
			func(Volume(d));
	}
	else
	{
		if (traversal_policy == CMapBase::TraversalPolicy::AUTO && is_indexed<Volume>(m))
		{
			CellMarkerStore<MESH, Volume> marker(m);
			foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
				Volume v(d);
				if constexpr (mesh_traits<MESH>::dimension == 3) // volumes can be boundary cells
				{
					if (!is_boundary(m, d) && !marker.is_marked(v))
					{
						marker.mark(v);
						return func(v);
					}
				}
				else
				{
					if (!marker.is_marked(v))
					{
						marker.mark(v);
						return func(v);
					}
				}
				return true;
			});
		}
		else
		{
			DartMarkerStore<MESH> marker(m);
			foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
				if constexpr (mesh_traits<MESH>::dimension == 3) // volumes can be boundary cells
				{
					if (!is_boundary(m, d) && !marker.is_marked(d))
					{
						Volume v(d);
						foreach_dart_of_orbit(m, v, [&](Dart d) -> bool {
							marker.mark(d);
							return true;
						});
						return func(v);
					}
				}
				else
				{
					if (!marker.is_marked(d))
					{
						Volume v(d);
						foreach_dart_of_orbit(m, v, [&](Dart d) -> bool {
							marker.mark(d);
							return true;
						});
						return func(v);
					}
				}
				return true;
			});
		}
	}
}


template <typename MESH, typename FUNC>
auto foreach_adjacent_vertex_through_edge(const MESH& m, typename mesh_traits<MESH>::Vertex v, const FUNC& func)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	foreach_adjacent_vertex_through_edge(m, v, func, CMapBase::TraversalPolicy::AUTO);
}

template <typename MESH, typename FUNC>
auto foreach_adjacent_vertex_through_edge(const MESH& m, typename mesh_traits<MESH>::Vertex v, const FUNC& func,
										  CMapBase::TraversalPolicy traversal_policy)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	using Vertex = typename mesh_traits<MESH>::Vertex;

	static_assert(is_func_parameter_same<FUNC, Vertex>::value, "Wrong function cell parameter type");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	if constexpr (std::is_convertible_v<MESH&, Graph&> && mesh_traits<MESH>::dimension == 1)
	{
		foreach_dart_of_orbit(m, v, [&](Dart d) -> bool { return func(Vertex(alpha0(m, d))); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap2&> && mesh_traits<MESH>::dimension == 2)
	{
		foreach_dart_of_orbit(m, v, [&](Dart d) -> bool { return func(Vertex(phi2(m, d))); });
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap3&> && mesh_traits<MESH>::dimension == 3)
	{
		if (traversal_policy == CMapBase::TraversalPolicy::AUTO && is_indexed<Vertex>(m))
		{
			CellMarkerStore<MESH, Vertex> marker(m);
			foreach_dart_of_orbit(m, v, [&](Dart d) -> bool {
				Vertex av(phi2(m, d));
				if (!marker.is_marked(av))
				{
					marker.mark(av);
					return func(av);
				}
				return true;
			});
		}
		else
		{
			DartMarkerStore<MESH> marker(m);
			foreach_dart_of_orbit(m, v, [&](Dart d) -> bool {
				Vertex av(phi2(m, d));
				if (!marker.is_marked(av.dart))
				{
					foreach_dart_of_orbit(m, av, [&](Dart d) -> bool {
						marker.mark(d);
						return true;
					});
					return func(v);
				}
				return true;
			});
		}
	}
}


template <typename MESH, typename FUNC>
auto foreach_adjacent_face_through_edge(const MESH& m, typename mesh_traits<MESH>::Face f, const FUNC& func)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	foreach_adjacent_face_through_edge(m, f, func, CMapBase::TraversalPolicy::AUTO);
}

template <typename MESH, typename FUNC>
auto foreach_adjacent_face_through_edge(const MESH& m, typename mesh_traits<MESH>::Face f, const FUNC& func,
										CMapBase::TraversalPolicy traversal_policy)
	-> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	using Face = typename mesh_traits<MESH>::Face;

	static_assert(is_func_parameter_same<FUNC, Face>::value, "Wrong function cell parameter type");
	static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

	if constexpr (std::is_convertible_v<MESH&, CMap2&> && mesh_traits<MESH>::dimension == 2)
	{
		foreach_dart_of_orbit(m, f, [&](Dart d) -> bool {
			if (!is_boundary(m, d))
				return func(Face(phi2(m, d)));
			return true;
		});
	}
	else if constexpr (std::is_convertible_v<MESH&, CMap3&> && mesh_traits<MESH>::dimension == 3)
	{
		using Face2 = typename mesh_traits<MESH>::Face2;
		using Edge = typename mesh_traits<MESH>::Edge;

		if (traversal_policy == CMapBase::TraversalPolicy::AUTO && is_indexed<Face>(m))
		{
			CellMarkerStore<MESH, Face> marker(m);
			marker.mark(f);
			foreach_dart_of_orbit(m, Face2(f.dart), [&](Dart d) -> bool {
				bool cont = true;
				foreach_incident_face(m, Edge(d), [&](Face iface) -> bool {
					if (!marker.is_marked(iface))
					{
						cont = func(iface);
						marker.mark(iface);
						return cont;
					}
					return true;
				});
				return cont;
			});
		}
		else
		{
			DartMarkerStore<MESH> marker(m);
			foreach_dart_of_orbit(m, f, [&](Dart d) -> bool {
				marker.mark(d);
				return true;
			});
			foreach_dart_of_orbit(m, Face2(f.dart), [&](Dart d) -> bool {
				bool cont = true;
				foreach_incident_face(m, Edge(d), [&](Face iface) -> bool {
					if (!marker.is_marked(iface.dart))
					{
						cont = func(iface);
						foreach_dart_of_orbit(m, iface, [&](Dart d) -> bool {
							marker.mark(d);
							return true;
						});
						return cont;
					}
					return true;
				});
				return cont;
			});
		}
	}
}






template <typename T, typename CELL, typename MESH,
		  typename std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>* = nullptr>
std::shared_ptr<typename mesh_traits<MESH>::template Attribute<T>> add_attribute(MESH& m, const std::string& name)
{
	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	if (!is_indexed<CELL>(m))
		index_cells<CELL>(m);
	CMapBase& mb = static_cast<CMapBase&>(m);
	return mb.attribute_containers_[CELL::ORBIT].template add_attribute<T>(name);
}


template <typename T, typename CELL, typename MESH,
		  typename std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>* = nullptr>
std::shared_ptr<CMapBase::Attribute<T>> get_attribute(const MESH& m, const std::string& name)
{
	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	return m.attribute_containers_[CELL::ORBIT].template get_attribute<T>(name);
}

template <typename CELL>
void remove_attribute(CMapBase& m, const std::shared_ptr<CMapBase::AttributeGen>& attribute)
{
	m.attribute_containers_[CELL::ORBIT].remove_attribute(attribute);
}

template <typename CELL>
void remove_attribute(CMapBase& m, CMapBase::AttributeGen* attribute)
{
	m.attribute_containers_[CELL::ORBIT].remove_attribute(attribute);
}

template <typename CELL, typename FUNC>
void foreach_attribute(const CMapBase& m, const FUNC& f)
{
	using AttributeGen = CMapBase::AttributeGen;
	static_assert(is_func_parameter_same<FUNC, const std::shared_ptr<AttributeGen>&>::value,
				  "Wrong function attribute parameter type");
	for (const std::shared_ptr<AttributeGen>& a : m.attribute_containers_[CELL::ORBIT])
		f(a);
}

template <typename T, typename CELL, typename FUNC>
void foreach_attribute(const CMapBase& m, const FUNC& f)
{
	using AttributeT = CMapBase::Attribute<T>;
	using AttributeGen = CMapBase::AttributeGen;
	static_assert(is_func_parameter_same<FUNC, const std::shared_ptr<AttributeT>&>::value,
				  "Wrong function attribute parameter type");
	for (const std::shared_ptr<AttributeGen>& a : m.attribute_containers_[CELL::ORBIT])
	{
		std::shared_ptr<AttributeT> at = std::dynamic_pointer_cast<AttributeT>(a);
		if (at)
			f(at);
	}
}

template <typename T>
T& get_attribute(CMapBase& m, const std::string& name)
{
	return m.get_attribute<T>(name);
}

void clear(CMapBase& m, bool keep_attributes = true);

//    template <typename MESH, typename std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>* = nullptr>
//    void copy(MESH& dst, const MESH& src)

void copy(CMapBase& dst, const CMapBase& src);


inline bool is_indexed(const CMapBase& m, Orbit orbit)
{
    cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
    return m.cells_indices_[orbit] != nullptr;
}

template <typename CELL>
bool is_indexed(const CMapBase& m)
{
    static const Orbit orbit = CELL::ORBIT;
    static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
    return m.cells_indices_[orbit] != nullptr;
}

template <typename CELL>
uint32 new_index(const CMapBase& m)
{
    return m.attribute_containers_[CELL::ORBIT].new_index();
}

template <typename CELL>
uint32 maximum_index(const CMapBase& m)
{
    static const Orbit orbit = CELL::ORBIT;
    static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
    cgogn_message_assert(is_indexed<CELL>(m), "Trying to access a cell index of an unindexed cell type");
    return m.attribute_containers_[CELL::ORBIT].maximum_index();
}


template <typename CELL>
uint32 index_of(const CMapBase& m, CELL c)
{
    static const Orbit orbit = CELL::ORBIT;
    static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
    cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
    return (*m.cells_indices_[orbit])[c.dart.index];
}


template <typename CELL>
CELL of_index(const CMapBase& m, uint32 i)
{
    static const Orbit orbit = CELL::ORBIT;
    static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
    cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
    for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
    {
        if (!is_boundary(m, d))
        {
            const CELL c(d);
            if (index_of(m, c) == i)
                return c;
        }
    }
    return CELL();
}

template <typename CELL>
void init_cells_indexing(CMapBase& m)
{
    static const Orbit orbit = CELL::ORBIT;
    static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
    if (!is_indexed<CELL>(m))
    {
        std::ostringstream oss;
        oss << "__index_" << orbit_name(orbit);
        m.cells_indices_[orbit] = m.darts_.add_attribute<uint32>(oss.str());
        m.cells_indices_[orbit]->fill(INVALID_INDEX);
    }
}

inline void init_cells_indexing(CMapBase& m, Orbit orbit)
{
    cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
    if (!is_indexed(m, orbit))
    {
        std::ostringstream oss;
        oss << "__index_" << orbit_name(orbit);
        m.cells_indices_[orbit] = m.darts_.add_attribute<uint32>(oss.str());
        m.cells_indices_[orbit]->fill(INVALID_INDEX);
    }
}

template <typename CELL, typename MESH>
auto set_index(MESH& m, CELL c, uint32 index) -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
    cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
    foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
        set_index<CELL>(m, d, index);
        return true;
    });
}

template <typename CELL, typename MESH>
auto index_cells(MESH& m) -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
    if (!is_indexed<CELL>(m))
        init_cells_indexing<CELL>(m);

    CMapBase& base = static_cast<CMapBase&>(m);
    DartMarker dm(m);
    for (Dart d = base.begin(), end = base.end(); d != end; d = base.next(d))
    {
        if (!is_boundary(m, d) && !dm.is_marked(d))
        {
            const CELL c(d);
            foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
                dm.mark(d);
                return true;
            });

            if (index_of(m, c) == INVALID_INDEX)
                set_index(m, c, new_index<CELL>(m));
        }
    }
}

template <typename CELL, typename MESH>
std::string cell_name(const MESH& /*m*/)
{
	static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
	return mesh_traits<MESH>::cell_names[tuple_type_index<CELL, typename mesh_traits<MESH>::Cells>::value];
}


template <typename CELL, typename MESH, typename std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>* = nullptr>
bool check_indexing(MESH& m, bool verbose = true)
{
    static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");

    if (!is_indexed<CELL>(m))
        return true;

    bool result = true;

    auto counter = add_attribute<uint32, CELL>(m, "__cell_counter");
    counter->fill(0);

    foreach_cell(
        m,
        [&](CELL c) -> bool {
            const uint32 index = index_of(m, c);

            ++(*counter)[index];

            bool valid_index = index != INVALID_INDEX;
            if (verbose && !valid_index)
                std::cerr << "Cell " << c << " (" << cell_name<CELL>(m) << ") has invalid index" << std::endl;

            bool all_darts_same_index = true;
            foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
                const uint32 index_d = index_of(m, CELL(d));
                if (index_d != index)
                {
                    if (verbose)
                        std::cerr << "Cell " << c << " (" << cell_name<CELL>(m) << ") has darts with different indices"
                                    << std::endl;
                    all_darts_same_index = false;
                }
                return true;
            });

            result &= valid_index && all_darts_same_index;
            return true;
        },
        CMapBase::TraversalPolicy::DART_MARKING);

    // check that all lines of the attribute container are used
    for (uint32 i = m.attribute_containers_[CELL::ORBIT].first_index(),
                end = m.attribute_containers_[CELL::ORBIT].last_index();
            i != end; i = m.attribute_containers_[CELL::ORBIT].next_index(i))
    {
        if ((*counter)[i] == 0)
        {
            if (verbose)
                std::cerr << "Cell index " << i << " is not used in container " << cell_name<CELL>(m) << std::endl;
            result = false;
        }
        else
        {
            if ((*counter)[i] >= 2ul)
            {
                if (verbose)
                    std::cerr << "Multiple cells with same index " << i << " in container " << cell_name<CELL>(m)
                                << std::endl;
                result = false;
            }
        }
    }

    remove_attribute<CELL>(m, counter);

    return result;
}

void dump_map_darts(const CMapBase& m);

    
inline bool is_boundary(const CMapBase& m, Dart d)
{
	return (*m.boundary_marker_)[d.index] != 0u;
}

inline uint32 nb_darts(const CMapBase& m)
{
	return m.darts_.nb_elements();
}

template <typename MESH, typename CELL>
auto is_incident_to_boundary(const MESH& m, CELL c) -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>, bool>
{
	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	bool result = false;
	foreach_dart_of_orbit(m, c, [&m, &result](Dart d) -> bool {
		if (is_boundary(m, d))
		{
			result = true;
			return false;
		}
		return true;
	});
	return result;
}


} // namespace cgogn

#endif // CGOGN_CORE_CMAP_CMAP_BASE_H_
