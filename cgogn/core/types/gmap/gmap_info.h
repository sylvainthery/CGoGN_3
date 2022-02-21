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

#ifndef CGOGN_CORE_GMAP_CMAP_INFO_H_
#define CGOGN_CORE_GMAP_CMAP_INFO_H_

#include <cgogn/core/types/gmap/gmap_base.h>
#include <cgogn/core/types/gmap/orbit_traversal.h>

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
// SAME_CMAP
template <typename CELL, typename GMAP>
uint32 nb_darts_of_orbit(const GMAP& m, CELL c)
{
	static_assert(is_in_tuple<CELL, typename GMAP::Cells>::value, "CELL not supported in this CMAP");
	uint32 result = 0;
	foreach_dart_of_orbit(m, c, [&](Dart) -> bool {
		++result;
		return true;
	});
	return result;
}

/*****************************************************************************/

// template <typename CMAP>
// void is_boundary(const CMAP& m, Dart d)

/*****************************************************************************/

// template <typename CMAP>
// uint32 nb_darts(const CMAP& m)

/*****************************************************************************/

//////////////
// GMapBase //
//////////////
// SAME_CMAP
inline uint32 nb_darts(const GMapBase& m)
{
	return m.darts_.nb_elements();
}

/*****************************************************************************/

// template <typename CMAP>
// void dump_map(const CMAP& m);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////

inline void dump_map_darts(const CMapBase& m)
{
	for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
	{
		std::cout << "index: " << std::setw(5) << d.index << " / ";
		for (auto& r : m.relations_)
			std::cout << r->name() << ": " << std::setw(5) << (*r)[d.index] << " / ";
		for (auto& ind : m.cells_indices_)
			if (ind)
				std::cout << ind->name() << ": " << std::setw(5) << (*ind)[d.index] << " / ";
	}
}

} // namespace cgogn

#endif // CGOGN_CORE_GMAP_CMAP_INFO_H_
