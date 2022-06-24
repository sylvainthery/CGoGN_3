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

#include <cgogn/core/types/cmap/cmap3.h>
#include <cgogn/core/types/cmap/cph3.h>
#include <cgogn/core/types/incidence_graph/incidence_graph_ops.h>

#include <cgogn/core/types/cmap/cmap_ops.h>
#include <cgogn/core/types/cmap/orbit_traversal.h>
#include <cgogn/core/functions/cells.h>
#include <cgogn/core/functions/mesh_info.h>
#include <cgogn/core/functions/traversals/edge.h>
#include <cgogn/core/functions/traversals/vertex.h>

namespace cgogn
{

/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Face
// add_face(MESH& m, uint32 size, bool set_indices = true);

/*****************************************************************************/

///////////
// CMap1 //
///////////

///////////
// CMap2 //
///////////


/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Face
// add_face(MESH& m, std::vector<typename mesh_traits<MESH>::Edge edges);

/*****************************************************************************/

////////////////////
// IncidenceGraph //
////////////////////

<<<<<<< HEAD
=======
IncidenceGraph::Face add_face(IncidenceGraph& ig, std::vector<IncidenceGraph::Edge>& edges)
{
	using Vertex = IncidenceGraph::Vertex;
	using Edge = IncidenceGraph::Edge;
	using Face = IncidenceGraph::Face;

	Face f = add_cell<Face>(ig);
	(*ig.face_incident_edges_)[f.index_] = edges;
	if (sort_face_edges(ig, f))
	{
		for (Edge e : edges)
			(*ig.edge_incident_faces_)[e.index_].push_back(f);
		return f;
	}
	else
	{
		remove_cell<Face>(ig, f);
		return Face();
	}
}

>>>>>>> compilVS
/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Face
// remove_face(MESH& m, typename mesh_traits<MESH>::Face f, bool set_indices = true);

/*****************************************************************************/

////////////////////
// IncidenceGraph //
////////////////////


///////////
// CMap1 //
///////////

/*****************************************************************************/

// template <typename MESH>
// void
// merge_incident_faces(MESH& m, typename mesh_traits<MESH>::Edge e, bool set_indices = true);

/*****************************************************************************/

///////////
// CMap2 //
///////////


/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Edge
// cut_face(MESH& m, typename mesh_traits<MESH>::Vertex v1, typename mesh_traits<MESH>::Vertex v2, bool set_indices =
// true);

/*****************************************************************************/

////////////////////
// IncidenceGraph //
////////////////////

<<<<<<< HEAD
=======
IncidenceGraph::Edge CGOGN_CORE_EXPORT cut_face(IncidenceGraph& ig, IncidenceGraph::Vertex v0,
												IncidenceGraph::Vertex v1)
{
	using Vertex = IncidenceGraph::Vertex;
	using Edge = IncidenceGraph::Edge;
	using Face = IncidenceGraph::Face;

	// TODO: manage face_incident_edges_dir_ !!!

	// find common face
	std::vector<Face> faces0 = incident_faces(ig, v0);
	std::vector<Face> faces1 = incident_faces(ig, v1);

	Face face;
	for (uint32 i = 0; i < faces0.size(); ++i)
	{
		for (uint32 j = 0; j < faces1.size(); ++j)
		{
			if (faces0[i] == faces1[j])
			{
				face = faces0[i];
				break;
			}
		}
		if (face.is_valid())
			break;
	}

	if (!face.is_valid())
		return Edge();

	std::vector<Edge>& edges = (*ig.face_incident_edges_)[face.index_];
	std::vector<Vertex> vertices = sorted_face_vertices(ig, face);

	std::vector<Edge> face_edge0;
	std::vector<Edge> face_edge1;

	bool inside = false;
	for (uint32 i = 0; i < edges.size(); ++i)
	{
		if (vertices[i] == v0 || vertices[i] == v1)
			inside = !inside;

		if (inside)
			face_edge1.push_back(edges[i]);
		else
			face_edge0.push_back(edges[i]);
	}

	remove_face(ig, face);
	Edge new_edge = add_edge(ig, v0, v1);
	face_edge0.push_back(new_edge);
	face_edge1.push_back(new_edge);
	add_face(ig, face_edge0);
	add_face(ig, face_edge1);

	return new_edge;
}

>>>>>>> compilVS
///////////
// CMap2 //
///////////

///////////
// CMap3 //
///////////

//////////
// CPH3 //
//////////

/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Face
// close_hole(MESH& m, Dart d, bool set_indices = true);

/*****************************************************************************/

///////////
// CMap2 //
///////////


/*****************************************************************************/

// template <typename MESH>
// void
// reverse_orientation(MESH& m);

/*****************************************************************************/

///////////
// CMap2 //
///////////


} // namespace cgogn
