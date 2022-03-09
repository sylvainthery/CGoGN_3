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
#include <cgogn/core/utils/numerics.h>
#include <cgogn/core/types/mesh_traits.h>
#include <cgogn/core/types/cmap/orbit_traversal.h>

#include <sstream>

namespace cgogn
{


//////////
// CPH3 //
//////////

template <typename MRMAP, typename CELL>
//inline auto index_of(const MRMAP& m, CELL c) -> std::enable_if_t<std::is_convertible_v<MRMAP&, CPH3&>, uint32>
inline auto index_of(const MRMAP& m, CELL c) -> std::enable_if_t<mesh_traits<MRMAP>::is_CPH3, uint32>
{
	static const Orbit orbit = CELL::ORBIT;

    if constexpr (orbit == MRMAP::CMAP::Edge::ORBIT)
		c.dart = m.edge_youngest_dart(c.dart);
    if constexpr (orbit == MRMAP::CMAP::Face::ORBIT)
		c.dart = m.face_youngest_dart(c.dart);
    if constexpr (orbit == MRMAP::CMAP::Volume::ORBIT)
		c.dart = m.volume_youngest_dart(c.dart);

    return index_of(static_cast<const typename MRMAP::CMAP&>(m), c);
}


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

template <typename CELL, typename MESH>
auto set_index(MESH& m, CELL c, uint32 index) -> std::enable_if_t<mesh_traits<MESH>::is_CMapBase>
{
	static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
	cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
	foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
		set_index<CELL>(m, d, index);
		return true;
	});
}

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
