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

#ifndef CGOGN_CORE_FUNCTIONS_CELLS_H_
#define CGOGN_CORE_FUNCTIONS_CELLS_H_

#include <cgogn/core/types/cmap/dart_marker.h>

#include <cgogn/core/types/cmap/cmap_base.h>
#include <cgogn/core/types/cmap/cmap_info.h>
#include <cgogn/core/types/cmap/cmap_ops.h>
#include <cgogn/core/types/incidence_graph/incidence_graph.h>

#include <sstream>

namespace cgogn
{

/*****************************************************************************/

// template <typename CELL>
// bool is_indexed(CMapBase& m);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////


////////////////////
// IncidenceGraph //
////////////////////


/*****************************************************************************/

// template <typename CELL, typename MESH>
// uint32 maximum_index(MESH& m);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////


/*****************************************************************************/

// template <typename CELL, typename MESH>
// uint32 index_of(MESH& m, CELL c);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////


////////////////////
// IncidenceGraph //
////////////////////



//////////
// CPH3 //
//////////


/*****************************************************************************/

// template <typename CELL, typename MESH>
// CELL of_index(MESH& m, uint32 i);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////



/*****************************************************************************/

// template <typename CELL, typename MESH>
// uint32 new_index(MESH& m);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////


////////////////////
// IncidenceGraph //
////////////////////

/*****************************************************************************/

// template <typename CELL, typename MESH>
// bool init_cells_indexing(MESH& m);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////




/*****************************************************************************/

// template <typename CELL, typename MESH>
// uint32 set_index(MESH& m, CELL c);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////


/*****************************************************************************/

// template <typename CELL, typename MESH>
// void index_cells(MESH& m);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////


/////////////
// GENERIC //
/////////////

// template <typename CELL, typename MESH>
// void index_cells(MESH& m)
// {
// 	static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
// 	if (!is_indexed<CELL>(m))
// 		init_cells_indexing<CELL>(m);

// 	foreach_cell(m, [&](CELL c) -> bool {
// 		if (index_of(m, c) == INVALID_INDEX)
// 			set_index(m, c, new_index<CELL>(m));
// 		return true;
// 	});
// }

} // namespace cgogn

#endif // CGOGN_CORE_FUNCTIONS_CELLS_H_
