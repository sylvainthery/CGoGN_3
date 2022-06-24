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

#include <cgogn/core/types/incidence_graph/incidence_graph_ops.h>

#include <cgogn/core/functions/cells.h>
#include <cgogn/core/functions/mesh_info.h>

#include <cgogn/core/functions/traversals/edge.h>
#include <cgogn/core/functions/traversals/vertex.h>


namespace cgogn
{

/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Edge
// add_edge(MESH& m, typename mesh_traits<MESH>::Vertex v0, typename mesh_traits<MESH>::Vertex v1);

/*****************************************************************************/


IncidenceGraph::Edge add_edge(IncidenceGraph& ig, IncidenceGraph::Vertex v0, IncidenceGraph::Vertex v1)
{
	using Edge = IncidenceGraph::Edge;

	Edge e = add_cell<Edge>(ig);
	(*ig.edge_incident_vertices_)[e.index_] = {v0, v1};
	(*ig.edge_incident_faces_)[e.index_].clear();
	(*ig.vertex_incident_edges_)[v0.index_].push_back(e);
	(*ig.vertex_incident_edges_)[v1.index_].push_back(e);

	return e;
}

/*****************************************************************************/

// template <typename MESH>
// void
// remove_edge(MESH& m, typename mesh_traits<MESH>::Edge e);

/*****************************************************************************/

////////////////////
// IncidenceGraph //
////////////////////

void remove_edge(IncidenceGraph& ig, IncidenceGraph::Edge e)
{
	//	using Vertex = IncidenceGraph::Vertex;
	using Edge = IncidenceGraph::Edge;

	while ((*ig.edge_incident_faces_)[e.index_].size() > 0)
		remove_face(ig, (*ig.edge_incident_faces_)[e.index_].back());

	auto [v0, v1] = (*ig.edge_incident_vertices_)[e.index_];
	remove_edge_in_vertex(ig, v0, e);
	remove_edge_in_vertex(ig, v1, e);

	remove_cell<Edge>(ig, e);
}

/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Vertex
// cut_edge(MESH& m, typename mesh_traits<MESH>::Edge e, bool set_indices = true);

/*****************************************************************************/

////////////////////
// IncidenceGraph //
////////////////////

IncidenceGraph::Vertex cut_edge(IncidenceGraph& ig, IncidenceGraph::Edge e0, bool /*set_indices*/)
{
	using Vertex = IncidenceGraph::Vertex;
	using Edge = IncidenceGraph::Edge;
	using Face = IncidenceGraph::Face;

	auto [v0, v1] = (*ig.edge_incident_vertices_)[e0.index_];
	Vertex v = add_cell<Vertex>(ig);
	(*ig.edge_incident_vertices_)[e0.index_] = {v0, v};
	Edge e1 = add_edge(ig, v, v1);
	for (Face f : (*ig.edge_incident_faces_)[e0.index_])
	{
		(*ig.face_incident_edges_)[f.index_].push_back(e1);
		sort_face_edges(ig, f); // TODO: could do more efficient (insert)
	}
	return v;
}


std::pair<IncidenceGraph::Vertex, std::vector<IncidenceGraph::Edge>> collapse_edge(IncidenceGraph& ig,
																				   IncidenceGraph::Edge e,
																				   bool /*set_indices*/)
{
	using Vertex = IncidenceGraph::Vertex;
	using Edge = IncidenceGraph::Edge;
	using Face = IncidenceGraph::Face;

	std::vector<Edge> removed_edges;

	auto [v1, v2] = (*ig.edge_incident_vertices_)[e.index_];

	// remove e from its incident vertices
	remove_edge_in_vertex(ig, v1, e);
	remove_edge_in_vertex(ig, v2, e);

	// remove e from its incident faces
	for (Face iface : (*ig.edge_incident_faces_)[e.index_])
	{
		remove_edge_in_face(ig, iface, e);
		// remove degenerate faces
		if ((*ig.face_incident_edges_)[iface.index_].size() < 3)
			remove_face(ig, iface);
	}

	// replace v1 by v2 in incident edges of v1
	for (Edge iev1 : (*ig.vertex_incident_edges_)[v1.index_])
	{
		replace_vertex_in_edge(ig, iev1, v1, v2);
		// check for duplicate edges around v2
		Edge similar_edge_in_v2;
		for (uint32 i = 0; !similar_edge_in_v2.is_valid() && i < (*ig.vertex_incident_edges_)[v2.index_].size(); ++i)
		{
			Edge iev2 = (*ig.vertex_incident_edges_)[v2.index_][i];
			if (same_edge(ig, iev1, iev2))
				similar_edge_in_v2 = iev2;
		}
		if (!similar_edge_in_v2.is_valid())
			(*ig.vertex_incident_edges_)[v2.index_].push_back(iev1);
		else
		{
			// migrate faces of iev1 to the similar edge in v2
			for (Face iface : (*ig.edge_incident_faces_)[iev1.index_])
			{
				auto fit = std::find((*ig.edge_incident_faces_)[similar_edge_in_v2.index_].begin(),
									 (*ig.edge_incident_faces_)[similar_edge_in_v2.index_].end(), iface);
				if (fit == (*ig.edge_incident_faces_)[similar_edge_in_v2.index_].end())
				{
					replace_edge_in_face(ig, iface, iev1, similar_edge_in_v2);
					(*ig.edge_incident_faces_)[similar_edge_in_v2.index_].push_back(iface);
				}
			}
			// remove iev1 from its vertices
			auto [iev1v1, iev1v2] = (*ig.edge_incident_vertices_)[iev1.index_];
			remove_edge_in_vertex(ig, iev1v1, iev1);
			remove_edge_in_vertex(ig, iev1v2, iev1);
			// remove iev1
			remove_cell<Edge>(ig, iev1);
			removed_edges.push_back(iev1);
		}
	}

	// remove v1
	remove_cell<Vertex>(ig, v1);
	// remove e
	remove_cell<Edge>(ig, e);

	return {v2, removed_edges};
}


IncidenceGraph::Face add_face(IncidenceGraph& ig, std::vector<IncidenceGraph::Edge>& edges)
{
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

void remove_face(IncidenceGraph& ig, IncidenceGraph::Face f)
{
	using Edge = IncidenceGraph::Edge;
	using Face = IncidenceGraph::Face;

	for (Edge e : (*ig.face_incident_edges_)[f.index_])
		remove_face_in_edge(ig, e, f);
	remove_cell<Face>(ig, f);
}


IncidenceGraph::Edge CGOGN_CORE_EXPORT cut_face(IncidenceGraph& ig, IncidenceGraph::Vertex v0,
												IncidenceGraph::Vertex v1)
{
	using Vertex = IncidenceGraph::Vertex;
	using Edge = IncidenceGraph::Edge;
	using Face = IncidenceGraph::Face;

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

IncidenceGraph::Vertex add_vertex(IncidenceGraph& ig)
{
	using Vertex = IncidenceGraph::Vertex;

	Vertex v = add_cell<Vertex>(ig);
	(*ig.vertex_incident_edges_)[v.index_].clear();
	return v;
}

void remove_vertex(IncidenceGraph& ig, IncidenceGraph::Vertex v)
{
	using Vertex = IncidenceGraph::Vertex;

	while ((*ig.vertex_incident_edges_)[v.index_].size() > 0)
		remove_edge(ig, (*ig.vertex_incident_edges_)[v.index_].back());
	remove_cell<Vertex>(ig, v);
}


IncidenceGraph::Edge connect_vertices(IncidenceGraph& ig, IncidenceGraph::Vertex v1, IncidenceGraph::Vertex v2)
{
	using Edge = IncidenceGraph::Edge;

	Edge e = add_cell<Edge>(ig);
	(*ig.edge_incident_vertices_)[e.index_] = {v1, v2};
	(*ig.edge_incident_faces_)[e.index_].clear();
	(*ig.vertex_incident_edges_)[v1.index_].push_back(e);
	(*ig.vertex_incident_edges_)[v2.index_].push_back(e);

	return e;
}


} // namespace cgogn


