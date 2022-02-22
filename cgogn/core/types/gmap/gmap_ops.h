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

#ifndef CGOGN_CORE_GMAP_CMAP_OPS_H_
#define CGOGN_CORE_GMAP_CMAP_OPS_H_

#include <cgogn/core/types/gmap/gmap_base.h>
#include <cgogn/core/types/cmap/orbit_traversal.h>

namespace cgogn
{

/*****************************************************************************/

// template <typename CMAP>
// Dart add_dart(CMAP& m);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////
// SAME_CMAP
inline Dart add_dart(GMapBase& m)
{
	uint32 index = m.darts_.new_index();
	Dart d(index);
	for (auto& rel : m.relations_)
		(*rel)[d.index] = d;
	for (auto& emb : m.cells_indices_)
		if (emb)
			(*emb)[d.index] = INVALID_INDEX;
	return d;
}

/*****************************************************************************/

// template <typename CMAP>
// void remove_dart(CMAP& m, Dart d);

/*****************************************************************************/

//////////////
// GMapBase //
//////////////
// SAME_CMAP
inline void remove_dart(CMapBase& m, Dart d)
{
	for (uint32 orbit = 0; orbit < NB_ORBITS; ++orbit)
	{
		if (m.cells_indices_[orbit])
		{
			uint32 index = (*m.cells_indices_[orbit])[d.index];
			if (index != INVALID_INDEX)
				m.attribute_containers_[orbit].unref_index(index);
		}
	}
	m.darts_.release_index(d.index);
}

/*****************************************************************************/

//////////////
// GMapBase //
//////////////
// SAME_CMAP
template <typename CELL>
void set_index(GMapBase& m, Dart d, uint32 index)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	const uint32 old = (*m.cells_indices_[orbit])[d.index];
	// ref_index() is done before unref_index() to avoid deleting the index if old == index
	if (index != INVALID_INDEX)
		m.attribute_containers_[orbit].ref_index(index); // ref the new index
	if (old != INVALID_INDEX)
		m.attribute_containers_[orbit].unref_index(old); // unref the old index
	(*m.cells_indices_[orbit])[d.index] = index;		 // affect the index to the dart
}

/*****************************************************************************/

// template <typename CELL, typename MESH>
// void copy_index(MESH& m, Dart dest, Dart src);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////

template <typename CELL, typename MESH>
auto copy_index(MESH& m, Dart dest, Dart src) -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	set_index<CELL>(m, dest, index_of(m, CELL(src)));
}

} // namespace cgogn

#endif // CGOGN_CORE_GMAP_CMAP_OPS_H_
