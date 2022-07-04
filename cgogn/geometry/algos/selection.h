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

#ifndef CGOGN_GEOMETRY_ALGOS_SELECTION_H_
#define CGOGN_GEOMETRY_ALGOS_SELECTION_H_

//#include <cgogn/core/types/map/cmap/cmap2.h>
#include <cgogn/core/types/map/dart_marker.h>
#include <cgogn/core/types/mesh_views/cell_cache.h>

#include <cgogn/geometry/functions/inclusion.h>
#include <cgogn/geometry/types/vector_traits.h>

#include <cgogn/core/functions/attributes.h>
#include <cgogn/core/functions/traversals/edge.h>
#include <cgogn/core/functions/traversals/face.h>
#include <cgogn/core/functions/traversals/vertex.h>

namespace cgogn
{

namespace geometry
{
template <typename MAP2>
CellCache<MAP2> within_sphere(const MAP2& m, typename MAP2::Vertex center, geometry::Scalar radius,
			   const typename MAP2::template Attribute<Vec3>* vertex_position)
{
	using Vertex = typename MAP2::Vertex;
	using HalfEdge = typename MAP2::HalfEdge;
	using Edge = typename MAP2::Edge;
	using Face = typename MAP2::Face;

	CellCache<MAP2> cache(m);

	const Vec3& center_position = value<Vec3>(m, vertex_position, center);

	DartMarkerStore dm(m);

	auto mark_vertex = [&](Vertex v) {
		foreach_dart_of_orbit(m, v, [&](Dart d) -> bool {
			// mark a dart of the vertex
			dm.mark(d);

			// check if the edge of d is now completely marked
			// (which means all the vertices of the edge are in the sphere)
			Edge e(d);
			bool all_in = true;
			foreach_dart_of_orbit(m, e, [&](Dart dd) -> bool {
				if (!dm.is_marked(dd))
					all_in = false;
				return all_in;
			});
			if (all_in)
				cache.add(e);

			// check if the face of d is now completely marked
			// (which means all the vertices of the face are in the sphere)
			Face f(d);
			all_in = true;
			foreach_dart_of_orbit(m, f, [&](Dart dd) -> bool {
				if (!dm.is_marked(dd))
					all_in = false;
				return all_in;
			});
			if (all_in)
				cache.add(f);

			return true;
		});
	};

	cache.add(center);
	mark_vertex(center);

	uint32 i = 0;
	while (i < cache.template cell_vector<Vertex>().size())
	{
		Vertex v = cache.template cell_vector<Vertex>()[i];
		foreach_adjacent_vertex_through_edge(m, v, [&](Vertex av) -> bool {
			const Vec3& p = value<Vec3>(m, vertex_position, av);
			if (in_sphere(p, center_position, radius))
			{
				if (!dm.is_marked(av.dart))
				{
					cache.add(av);
					mark_vertex(av);
				}
			}
			else
				cache.add(HalfEdge(phi2(m, av.dart)));
			return true;
		});
		++i;
	}

	return cache;
}

} // namespace geometry

} // namespace cgogn

#endif // CGOGN_GEOMETRY_ALGOS_SELECTION_H_
