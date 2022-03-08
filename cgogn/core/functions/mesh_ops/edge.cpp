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

#include <cgogn/core/functions/cells.h>
#include <cgogn/core/functions/mesh_info.h>

#include <cgogn/core/types/cmap/cmap_ops.h>
#include <cgogn/core/types/incidence_graph/incidence_graph_ops.h>

#include "cgogn/core/types/cmap/cmap1.h"
#include "cgogn/core/types/cmap/graph.h"


namespace cgogn
{

/*****************************************************************************/

// template <typename MESH>
// typename mesh_traits<MESH>::Edge
// add_edge(MESH& m, typename mesh_traits<MESH>::Vertex v0, typename mesh_traits<MESH>::Vertex v1);

/*****************************************************************************/

////////////////////
// IncidenceGraph //
////////////////////


/*****************************************************************************/

// template <typename MESH>
// void
// remove_edge(MESH& m, typename mesh_traits<MESH>::Edge e);

/*****************************************************************************/

////////////////////
// IncidenceGraph //
////////////////////




//GMap1::Vertex cut_edge(GMap1& m, GMap1::Edge e, bool set_indices)
//{
//    Dart d = add_dart(m);
//    phi1_sew(m, e.dart, d);
//    CMap1::Vertex v(d);

//    if (set_indices)
//    {
//        if (is_indexed<CMap1::Vertex>(m))
//            set_index(m, v, new_index<CMap1::Vertex>(m));
//        // CMap1::Edge is the same orbit as CMap1::Vertex
//        if (is_indexed<CMap1::Face>(m))
//            copy_index<CMap1::Face>(m, d, e.dart);
//    }

//    return v;
//}


} // namespace cgogn
