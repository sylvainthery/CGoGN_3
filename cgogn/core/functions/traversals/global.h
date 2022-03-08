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

#ifndef CGOGN_CORE_FUNCTIONS_TRAVERSALS_GLOBAL_H_
#define CGOGN_CORE_FUNCTIONS_TRAVERSALS_GLOBAL_H_

//#include <cgogn/core/utils/buffers.h>
//#include <cgogn/core/utils/thread.h>
//#include <cgogn/core/utils/thread_pool.h>
//#include <cgogn/core/utils/tuples.h>
#include <cgogn/core/utils/type_traits.h>
#include <cgogn/core/types/mesh_traits.h>

//#include <cgogn/core/types/cmap/cmap_info.h>
//#include <cgogn/core/types/cmap/dart_marker.h>
//#include <cgogn/core/types/cmap/orbit_traversal.h>
//#include <cgogn/core/types/cell_marker.h>

namespace cgogn
{

template <typename MESH, typename CELL>
std::vector<typename mesh_traits<MESH>::Vertex> incident_vertices(const MESH& m, CELL c)
{
    using Vertex = typename mesh_traits<MESH>::Vertex;
    std::vector<Vertex> vertices;
    vertices.reserve(32u);
    foreach_incident_vertex(m, c, [&](Vertex v) -> bool {
        vertices.push_back(v);
        return true;
    });
    return vertices;
}


template <typename MESH, typename CELL>
void append_incident_vertices(const MESH& m, CELL c, std::vector<typename mesh_traits<MESH>::Vertex>& vertices)
{
    using Vertex = typename mesh_traits<MESH>::Vertex;
    foreach_incident_vertex(m, c, [&vertices](Vertex v) -> bool {
        vertices.push_back(v);
        return true;
    });
}

template <typename MESH>
std::vector<typename mesh_traits<MESH>::Vertex> adjacent_vertices_through_edge(const MESH& m,
                                                                               typename mesh_traits<MESH>::Vertex v)
{
    using Vertex = typename mesh_traits<MESH>::Vertex;
    std::vector<Vertex> vertices;
    vertices.reserve(32u);
    foreach_adjacent_vertex_through_edge(m, v, [&](Vertex av) -> bool {
        vertices.push_back(av);
        return true;
    });
    return vertices;
}



template <typename MESH, typename CELL>
std::vector<typename mesh_traits<MESH>::Edge> incident_edges(const MESH& m, CELL c)
{
    using Edge = typename mesh_traits<MESH>::Edge;
    std::vector<Edge> edges;
    edges.reserve(32u);
    foreach_incident_edge(m, c, [&](Edge e) -> bool {
        edges.push_back(e);
        return true;
    });
    return edges;
}


template <typename MESH, typename CELL>
std::vector<typename mesh_traits<MESH>::Face> incident_faces(const MESH& m, CELL c)
{
    using Face = typename mesh_traits<MESH>::Face;
    std::vector<Face> faces;
    faces.reserve(32u);
    foreach_incident_face(m, c, [&](Face f) -> bool {
        faces.push_back(f);
        return true;
    });
    return faces;
}

template <typename MESH, typename CELL>
std::vector<typename mesh_traits<MESH>::Volume> incident_volumes(const MESH& m, CELL c)
{
    using Volume = typename mesh_traits<MESH>::Volume;
    if constexpr (mesh_traits<MESH>::dimension == 2)
        return {Volume(c.dart)};
    else
    {
        std::vector<Volume> volumes;
        volumes.reserve(32u);
        foreach_incident_volume(m, c, [&](Volume v) -> bool {
            volumes.push_back(v);
            return true;
        });
        return volumes;
    }
}


template <typename MESH, typename CELL>
std::vector<typename mesh_traits<MESH>::HalfEdge> incident_halfedges(const MESH& m, CELL c)
{
    using HalfEdge = typename mesh_traits<MESH>::HalfEdge;
    std::vector<HalfEdge> halfedges;
    halfedges.reserve(32u);
    foreach_incident_halfedge(m, c, [&](HalfEdge e) -> bool {
        halfedges.push_back(e);
        return true;
    });
    return halfedges;
}
} // namespace cgogn

#endif // CGOGN_CORE_FUNCTIONS_TRAVERSALS_GLOBAL_H_
