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

#ifndef CGOGN_CORE_CMAP_CMAP_INFO_H_
#define CGOGN_CORE_CMAP_CMAP_INFO_H_

#include <cgogn/core/types/map/cmap/orbit_traversal.h>

#include <iomanip>

namespace cgogn
{

/*****************************************************************************/

// template <typename CELL, typename CMAP>
// uint32 nb_darts_of_orbit(const CMAP& m, CELL c);

/*****************************************************************************/

/////////////
// GENERIC //
/////////////

template <typename CELL, typename CMAP>
uint32 nb_darts_of_orbit(const CMAP& m, CELL c)
{
	static_assert(is_in_tuple<CELL, typename CMAP::Cells>::value, "CELL not supported in this CMAP");
	uint32 result = 0;
	foreach_dart_of_orbit(m, c, [&](Dart) -> bool {
		++result;
		return true;
	});
	return result;
}

template <typename CMAP>
void copy(CMAP& dst, const CMAP& src)
{
	clear(dst, false);
	for (uint32 orbit = 0; orbit < NB_ORBITS; ++orbit)
	{
		if (src.cells_indices_[orbit] != nullptr)
			init_cells_indexing(dst, Orbit(orbit));
	}
	dst.darts_.copy(src.darts_);
	for (uint32 i = 0; i < NB_ORBITS; ++i)
		dst.attribute_containers_[i].copy(src.attribute_containers_[i]);
	dst.boundary_marker_ = dst.darts_.get_mark_attribute();
	dst.boundary_marker_->copy(*src.boundary_marker_);
}



} // namespace cgogn

#endif // CGOGN_CORE_CMAP_CMAP_INFO_H_
