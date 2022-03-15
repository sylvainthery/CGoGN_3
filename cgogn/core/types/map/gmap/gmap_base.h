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

#ifndef CGOGN_CORE_MAP_GMAP_GMAP_BASE_H_
#define CGOGN_CORE_MAP_GMAP_GMAP_BASE_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/map/map_base.h>

namespace cgogn
{

struct CGOGN_CORE_EXPORT GMapBase: public MapBase
{

};


inline bool is_boundary(const GMapBase&, Dart)
{
	return false;
}

inline void set_boundary(const GMapBase& , Dart) { }


///
/// \brief of_index Get the cell of type CELL of a given index
/// \param m
/// \param i
/// \return
///
template <typename CELL>
CELL of_index(const GMapBase& m, uint32 i)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
	for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
	{
		const CELL c(d);
		if (index_of(m, c) == i)
			return c;
	}
	return CELL();
}

/////
/////// \brief init_cells_indexing Add an index atttribute on dart for CELL embedding
/////// \param m
///////
//template <typename CELL>
//void init_cells_indexing(MapBase& m)
//{
//	static const Orbit orbit = CELL::ORBIT;
//	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
//	if (!is_indexed<CELL>(m))
//	{
//		std::ostringstream oss;
//		oss << "__index_" << orbit_name(m,orbit);
//		m.cells_indices_[orbit] = m.darts_.add_attribute<uint32>(oss.str());
//		m.cells_indices_[orbit]->fill(INVALID_INDEX);
//	}
//}

///////
/////// \brief init_cells_indexing Add an index attribute on dart for orbit embedding
/////// \param m
/////// \param orbit
///////
//inline void init_cells_indexing(GMapBase& m, Orbit orbit)
//{
//	cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
//	if (!is_indexed(m, orbit))
//	{
//		std::ostringstream oss;
//		oss << "__index_" << orbit_name(m,orbit);
//		m.cells_indices_[orbit] = m.darts_.add_attribute<uint32>(oss.str());
//		m.cells_indices_[orbit]->fill(INVALID_INDEX);
//	}
//}


} // namespace cgogn

#endif // CGOGN_CORE_CMAP_CMAP_BASE_H_
