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

#ifndef CGOGN_CORE_MAP_OPS_H_
#define CGOGN_CORE_MAP_OPS_H_

#include <cgogn/core/types/map/dart.h>
#include <cgogn/core/utils/type_traits.h>
#include <cgogn/core/types/mesh_traits.h>

namespace cgogn
{

template <typename MESH_MAP>
auto add_pyramid(MESH_MAP& m, uint32 size, bool set_indices = true) ->
	std::enable_if_t< std::is_same_v<MESH_MAP&, struct CMap2&> || std::is_same_v<MESH_MAP&, struct GMap2&>, typename MESH_MAP::Volume >
{
	using ParentDimType = typename mesh_traits<MESH_MAP>::ParentDimType;
	using Vertex = typename mesh_traits<MESH_MAP>::Vertex;
	using HalfEdge = typename mesh_traits<MESH_MAP>::HalfEdge;
	using Edge = typename mesh_traits<MESH_MAP>::Edge;
	using Face = typename mesh_traits<MESH_MAP>::Face;
	using Face1 = typename mesh_traits<ParentDimType>::Face;
	using Volume = typename mesh_traits<MESH_MAP>::Volume;

	Face1 first = add_face(static_cast<ParentDimType&>(m), 3u, false); // First triangle
	Dart current = first.dart;
	for (uint32 i = 1u; i < size; ++i) // Next triangles
	{
		Face1 next = add_face(static_cast<ParentDimType&>(m), 3u, false);
		phi2_sew(m, phi_1(m, current), phi1(m, next.dart));
		current = next.dart;
	}
	phi2_sew(m, phi_1(m, current), phi1(m, first.dart)); // Finish the umbrella
	Face base = close_hole(m, first.dart, false); // Add the base face

	Volume vol(base.dart);

	if (set_indices)
	{
		if (is_indexed<Vertex>(m))
		{
			foreach_incident_vertex(
				m, vol,
				[&](Vertex v) -> bool {
					set_index(m, v, new_index<Vertex>(m));
					return true;
				},
				MapBase_TraversalPolicy::DART_MARKING);
		}
		if (is_indexed<HalfEdge>(m))
		{
			foreach_incident_edge(
				m, vol,
				[&](Edge e) -> bool {
					set_index(m, HalfEdge(e.dart), new_index<HalfEdge>(m));
					set_index(m, HalfEdge(phi2(m, e.dart)), new_index<HalfEdge>(m));
					return true;
				},
				MapBase_TraversalPolicy::DART_MARKING);
		}
		if (is_indexed<Edge>(m))
		{
			foreach_incident_edge(
				m, vol,
				[&](Edge e) -> bool {
					set_index(m, e, new_index<Edge>(m));
					return true;
				},
				MapBase_TraversalPolicy::DART_MARKING);
		}
		if (is_indexed<Face>(m))
		{
			foreach_incident_face(
				m, vol,
				[&](Face f) -> bool {
					set_index(m, f, new_index<Face>(m));
					return true;
				},
				MapBase_TraversalPolicy::DART_MARKING);
		}
		if (is_indexed<Volume>(m))
			set_index(m, vol, new_index<Volume>(m));
	}

	return vol;
}

template <typename MESH_MAP>
auto add_prism(MESH_MAP& m, uint32 size, bool set_indices= true)	->
	std::enable_if_t< std::is_same_v<MESH_MAP&, struct CMap2&> || std::is_same_v<MESH_MAP&, struct GMap2&>, typename MESH_MAP::Volume >
{

	using ParentDimType = typename mesh_traits<MESH_MAP>::ParentDimType;
	using Vertex = typename mesh_traits<MESH_MAP>::Vertex;
	using HalfEdge = typename mesh_traits<MESH_MAP>::HalfEdge;
	using Edge = typename mesh_traits<MESH_MAP>::Edge;
	using Face = typename mesh_traits<MESH_MAP>::Face;
	using Face1 = typename mesh_traits<ParentDimType>::Face;
	using Volume = typename mesh_traits<MESH_MAP>::Volume;

	Face1 first = add_face(static_cast<ParentDimType&>(m), 4u, false); // first quad
	Dart current = first.dart;
	for (uint32 i = 1u; i < size; ++i) // Next quads
	{
		Face1 next = add_face(static_cast<ParentDimType&>(m), 4u, false);
		phi2_sew(m, phi_1(m, current), phi1(m, next.dart));
		current = next.dart;
	}
	phi2_sew(m, phi_1(m, current), phi1(m, first.dart)); // Finish the sides
	Face base = close_hole(m, first.dart, false); // Add the base face
	close_hole(m, phi<1, 1>(m, first.dart), false);		 // Add the top face

	Volume vol(base.dart);

	if (set_indices)
	{
		if (is_indexed<Vertex>(m))
		{
			foreach_incident_vertex(
				m, vol,
				[&](Vertex v) -> bool {
					set_index(m, v, new_index<Vertex>(m));
					return true;
				},
				MapBase_TraversalPolicy::DART_MARKING);
		}
		if (is_indexed<HalfEdge>(m))
		{
			foreach_incident_edge(
				m, vol,
				[&](Edge e) -> bool {
					set_index(m, HalfEdge(e.dart), new_index<HalfEdge>(m));
					set_index(m, HalfEdge(phi2(m, e.dart)), new_index<HalfEdge>(m));
					return true;
				},
				MapBase_TraversalPolicy::DART_MARKING);
		}
		if (is_indexed<Edge>(m))
		{
			foreach_incident_edge(
				m, vol,
				[&](Edge e) -> bool {
					set_index(m, e, new_index<Edge>(m));
					return true;
				},
				MapBase_TraversalPolicy::DART_MARKING);
		}
		if (is_indexed<Face>(m))
		{
			foreach_incident_face(
				m, vol,
				[&](Face f) -> bool {
					set_index(m, f, new_index<Face>(m));
					return true;
				},
				MapBase_TraversalPolicy::DART_MARKING);
		}
		if (is_indexed<Volume>(m))
			set_index(m, vol, new_index<Volume>(m));
	}

	return vol;
}


template <typename MESH_MAP>
typename MESH_MAP::Volume add_tetrahedron(MESH_MAP& m, bool set_indices = true)
{
	return add_pyramid(m, 3, set_indices);
}

template <typename MESH_MAP>
typename MESH_MAP::Volume add_hexahedron(MESH_MAP& m, bool set_indices = true)
{
	return add_prism(m, 4, set_indices);
}




//
//template <typename CELL, typename MESH>
//auto copy_index(MES"H& m, Dart dest, Dart src)
//	-> std::enable_if_t<std::is_convertible_v<MESH&, struct MapBase&>>
//{
//	static const Orbit orbit = CELL::ORBIT;
//	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
//	set_index<CELL>(m, dest, index_of(m, CELL(src)));
//}

} // namespace cgogn

#endif // CGOGN_CORE_MAP_OPS_H_
