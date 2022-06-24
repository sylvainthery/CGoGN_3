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
#include <cgogn/core/types/cmap/graph.h>
#include <cgogn/core/types/incidence_graph/incidence_graph_ops.h>

#include <cgogn/core/functions/cells.h>
#include <cgogn/core/functions/mesh_ops/edge.h>
#include <cgogn/core/functions/mesh_ops/vertex.h>

#include <cgogn/core/types/cmap/cmap_info.h>
#include <cgogn/core/types/cmap/cmap_ops.h>

namespace cgogn
{

/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Vertex
// add_vertex(MESH& m, bool set_indices = true);

/*****************************************************************************/

///////////
// Graph //
///////////


////////////////////
// IncidenceGraph //
////////////////////


/*****************************************************************************/

// template <typename MESH>
// void
// remove_vertex(MESH& m, typename mesh_traits<MESH>::Vertex v, bool set_indices = true);

/*****************************************************************************/

///////////
// Graph //
///////////


////////////////////
// IncidenceGraph //
////////////////////



/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Edge
// connect_vertices(MESH& m, typename mesh_traits<MESH>::Vertex v1, typename mesh_traits<MESH>::Vertex v2, bool
// set_indices = true);

/*****************************************************************************/

///////////
// Graph //
///////////
<<<<<<< HEAD
=======

Graph::Edge connect_vertices(Graph& g, Graph::Vertex v1, Graph::Vertex v2, bool set_indices)
{
	static auto is_isolated = [](Graph& g, Graph::Vertex v) -> bool { return alpha0(g, v.dart) == alpha1(g, v.dart); };

	Dart d = v1.dart;
	Dart e = v2.dart;
	Dart dd = alpha0(g, d);
	Dart ee = alpha0(g, e);
	if (is_isolated(g, v1))
	{
		if (is_isolated(g, v2))
		{
			alpha1_unsew(g, d);
			alpha1_unsew(g, e);
			remove_dart(g, dd);
			remove_dart(g, ee);
			alpha0_sew(g, d, e);
			if (set_indices)
			{
				if (is_indexed<Graph::Edge>(g))
					copy_index<Graph::Edge>(g, e, d);
			}
			return Graph::Edge(d);
		}
		else
		{
			alpha1_unsew(g, d);
			alpha1_sew(g, e, dd);
			if (set_indices)
			{
				if (is_indexed<Graph::Vertex>(g))
					copy_index<Graph::Vertex>(g, dd, e);
			}
			return Graph::Edge(d);
		}
	}
	else
	{
		if (is_isolated(g, v2))
		{
			alpha1_unsew(g, e);
			alpha1_sew(g, d, ee);
			if (set_indices)
			{
				if (is_indexed<Graph::Vertex>(g))
					copy_index<Graph::Vertex>(g, ee, d);
			}
			return Graph::Edge(ee);
		}
		else
		{
			Dart ddd = add_dart(g);
			Dart eee = add_dart(g);
			alpha0_sew(g, ddd, eee);
			alpha1_sew(g, d, ddd);
			alpha1_sew(g, e, eee);
			if (set_indices)
			{
				if (is_indexed<Graph::Vertex>(g))
				{
					copy_index<Graph::Vertex>(g, ddd, d);
					copy_index<Graph::Vertex>(g, eee, e);
				}
				if (is_indexed<Graph::HalfEdge>(g))
				{
					set_index(g, Graph::HalfEdge(ddd), new_index<Graph::HalfEdge>(g));
					set_index(g, Graph::HalfEdge(eee), new_index<Graph::HalfEdge>(g));
				}
				if (is_indexed<Graph::Edge>(g))
					set_index(g, Graph::Edge(ddd), new_index<Graph::Edge>(g));
			}
			return Graph::Edge(ddd);
		}
	}
}

////////////////////
// IncidenceGraph //
////////////////////

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

>>>>>>> compilVS
/*****************************************************************************/

// template <typename MESH>
// void
// disconnect_vertices(MESH& m, typename mesh_traits<MESH>::Edge e, bool set_indices = true);

/*****************************************************************************/

///////////
// Graph //
///////////

/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Edge
// merge_vertices(MESH& m, typename mesh_traits<MESH>::Vertex v1, typename mesh_traits<MESH>::Vertex v2, bool
// set_indices = true);

/*****************************************************************************/

///////////
// Graph //
///////////

} // namespace cgogn
