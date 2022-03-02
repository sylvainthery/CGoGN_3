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

#ifndef CGOGN_CORE_FUNCTIONS_ATTRIBUTES_H_
#define CGOGN_CORE_FUNCTIONS_ATTRIBUTES_H_

#include <cgogn/core/functions/cells.h>
#include <cgogn/core/functions/traversals/global.h>

#include <cgogn/core/types/cmap/cmap_base.h>
#include <cgogn/core/types/incidence_graph/incidence_graph.h>

#include <cgogn/core/utils/tuples.h>

#include <string>

namespace cgogn
{

/*****************************************************************************/

// template <typename T, typename CELL, typename MESH>
// std::shared_ptr<typename mesh_traits<MESH>::template Attribute<T>> add_attribute(MESH& m, const std::string& name);

/*****************************************************************************/


/*****************************************************************************/

// template <typename T, typename CELL, typename MESH>
// std::shared_ptr<typename mesh_traits<MESH>::template Attribute<T>> get_attribute(const MESH& m, const std::string&
// name);

/*****************************************************************************/

//////////////
// CMapBase //
//////////////



////////////////////
// IncidenceGraph //
////////////////////


/*****************************************************************************/

// template <typename T, typename CELL, typename MESH>
// std::shared_ptr<typename mesh_traits<MESH>::template Attribute<T>> get_or_add_attribute(MESH& m, const
// std::string& name);

/*****************************************************************************/

/////////////
// GENERIC //
/////////////

/*****************************************************************************/

// template <typename CELL, typename MESH>
// void remove_attribute(MESH& m, std::shared_ptr<typename mesh_traits<MESH>::AttributeGen> attribute)

// template <typename CELL, typename MESH>
// void remove_attribute(MESH& m, typename mesh_traits<MESH>::AttributeGen* attribute)

/*****************************************************************************/


/*****************************************************************************/

// template <typename CELL, typename MESH, typename FUNC>
// void foreach_attribute(const MESH& m, const FUNC& f);

/*****************************************************************************/



/*****************************************************************************/

// template <typename T, typename CELL, typename MESH, typename FUNC>
// void foreach_attribute(const MESH& m, const FUNC& f);

/*****************************************************************************/




/*****************************************************************************/

// template <typename T, typename CELL, typename MESH>
// T& value(MESH& m, typename mesh_traits<MESH>::template AttributePtr<T> attribute, CELL c);

/*****************************************************************************/

/////////////
// GENERIC //
/////////////


template <typename T, typename CELL, typename MESH>
std::shared_ptr<typename mesh_traits<MESH>::template Attribute<T>> get_or_add_attribute(MESH& m,
																						const std::string& name)
{
	auto attribute = get_attribute<T, CELL>(m, name);
	if (!attribute)
		return add_attribute<T, CELL>(m, name);
	else
		return attribute;
}


template <typename T, typename CELL, typename MESH>
inline T& value(const MESH& m, const std::shared_ptr<typename mesh_traits<MESH>::template Attribute<T>>& attribute,
				CELL c)
{
	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	return (*attribute)[index_of(m, c)];
}

template <typename T, typename CELL, typename MESH>
inline T& value(const MESH& m, typename mesh_traits<MESH>::template Attribute<T>* attribute, CELL c)
{
	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	return (*attribute)[index_of(m, c)];
}

template <typename T, typename CELL, typename MESH>
inline const T& value(const MESH& m, const typename mesh_traits<MESH>::template Attribute<T>* attribute, CELL c)
{
	static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
	return (*attribute)[index_of(m, c)];
}



} // namespace cgogn

#endif // CGOGN_CORE_FUNCTIONS_ATTRIBUTES_H_
