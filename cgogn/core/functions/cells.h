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

#include <cgogn/core/types/map/dart_marker.h>
#include <cgogn/core/types/map/map_info.h>

#include <sstream>

namespace cgogn
{

///
/// \brief init_cells_indexing Add an index atttribute on dart for CELL embedding
/// \param m
///
template <typename CELL, typename MESH>
auto init_cells_indexing(MESH& m) -> std::enable_if_t<std::is_base_of_v<struct MapBase&, MESH&>>
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	if (!is_indexed<CELL>(m))
	{
		std::ostringstream oss;
		oss << "__index_" << orbit_name(m, orbit);
		m.cells_indices_[orbit] = m.darts_.template add_attribute<uint32>(oss.str());
		m.cells_indices_[orbit]->fill(INVALID_INDEX);
	}
}

///
/// \brief init_cells_indexing Add an index attribute on dart for orbit embedding
/// \param m
/// \param orbit
///
template <typename MESH>
auto init_cells_indexing(MESH& m, Orbit orbit) -> void //std::enable_if_t<std::is_convertible_v<MESH*, struct MapBase*>>
{
	static_assert(std::is_convertible_v<MESH*,struct MapBase*>, "must be MapBase");
	cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	if (!is_indexed(m, orbit))
	{
		std::ostringstream oss;
		oss << "__index_" << orbit_name(m,orbit);
		m.cells_indices_[orbit] = m.darts_.template add_attribute<uint32>(oss.str());
		m.cells_indices_[orbit]->fill(INVALID_INDEX);
	}
}




template <typename CELL, typename MESH>
auto set_index(MESH& m, CELL c, uint32 index) 
-> std::enable_if_t<std::is_convertible_v<MESH&, struct MapBase&>>
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
// MapBase //
//////////////

template <typename CELL, typename MESH>
auto index_cells(MESH& m) -> std::enable_if_t<std::is_convertible_v<MESH&, struct MapBase&>>
{
	static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
	if (!is_indexed<CELL>(m))
		init_cells_indexing<CELL>(m);

	typename mesh_traits<MESH>::BaseType& base = static_cast<typename mesh_traits<MESH>::BaseType&>(m);
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

} // namespace cgogn

#endif // CGOGN_CORE_FUNCTIONS_CELLS_H_
