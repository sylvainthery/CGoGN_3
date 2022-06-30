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
#include <cgogn/core/types/map/cmap/cmap_base.h>
#include <iomanip>

namespace cgogn
{
MapBase::MapBase()
{
	cells_embedded_orbit_.reserve(NB_ORBITS);
}

MapBase::~MapBase()
{
}


Dart add_dart(MapBase& m)
{
	uint32 index = m.darts_.new_index();
	Dart d(index);
	for (auto& rel : m.relations_)
		(*rel)[d.index] = d;
	for (uint32 orb : m.cells_embedded_orbit_)
	{
		auto& emb = m.cells_indices_[orb];
		(*emb)[d.index] = INVALID_INDEX;
	}
	return d;
}

void remove_dart(MapBase& m, Dart d)
{
//	for (uint32 orbit = 0; orbit < NB_ORBITS; ++orbit)
//	{
//		if (m.cells_indices_[orbit])
//		{
//			uint32 index = (*m.cells_indices_[orbit])[d.index];
//			if (index != INVALID_INDEX)
//				m.attribute_containers_[orbit].unref_index(index);
//		}
//	}
	for (uint32 orbit : m.cells_embedded_orbit_)
	{
		uint32 index = (*m.cells_indices_[orbit])[d.index];
		if (index != INVALID_INDEX)
			m.attribute_containers_[orbit].unref_index(index);
	}
	m.darts_.release_index(d.index);
}



void clear(MapBase& m, bool keep_attributes)
{
	// clear darts and keep attributes (phi relations)
	m.darts_.clear_attributes();
	if (!keep_attributes)
	{
		// remove cells indices attributes
//		for (uint32 orbit = 0; orbit < NB_ORBITS; ++orbit)
//		{
		for(uint32 orbit: m.cells_embedded_orbit_)
		{
			if (m.cells_indices_[orbit] != nullptr)
			{
				m.darts_.remove_attribute(m.cells_indices_[orbit]);
				m.cells_indices_[orbit].reset();
			}
		}
	}

	// clear all cell attributes
	for (MapBase::AttributeContainer& container : m.attribute_containers_)
	{
		if (keep_attributes)
			container.clear_attributes();
		else
		{
			container.clear_attributes();
			// if there are still shared_ptr somewhere, some attributes may not be removed
			container.remove_attributes();
		}
	}
}



} // namespace cgogn


