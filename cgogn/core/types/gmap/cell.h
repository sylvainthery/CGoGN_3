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

#ifndef CGOGN_CORE_TYPES_GMAP_CELL_H_
#define CGOGN_CORE_TYPES_GMAP_CELL_H_

#include <cgogn/core/types/cmap/dart.h>

#include <cgogn/core/utils/assert.h>
#include <cgogn/core/utils/numerics.h>

/**
 * \file cgogn/core/types/cmap/cell.h
 * \brief GM_Orbit and cell definitions used in cgogn.
 */

namespace cgogn
{

enum GM_Orbit : uint32
{
	DART = 0,
	BETA0,// 1EDGE 
	BETA1,// 1VERTEX
	BETA0_BETA1, // 2FACE = 1CC
	BETA0_BETA2, // 2EDGE
	BETA1_BETA2, // 2VERTEX
	BETA0_BETA1_BETA2, // 3VOLUME = 2CC
	BETA0_BETA1_BETA3, // 3FACE
	BETA0_BETA2_BETA3, // 3EDGE
	BETA1_BETA2_BETA3, // 3VERTEX
	BETA0_BETA1_BETA2_BETA3 // 3CC
};

static const std::size_t NB_GM_ORBITS = GM_Orbit::BETA0_BETA1_BETA2_BETA3 + 1;

inline std::string orbit_name(GM_Orbit orbit)
{
	switch (orbit)
	{
	case GM_Orbit::DART:
		return "cgogn::GM_Orbit::DART";
	case GM_Orbit::BETA1:
		return "cgogn::GM_Orbit::BETA2";
	case GM_Orbit::BETA2:
		return "cgogn::GM_Orbit::BETA2";
	case GM_Orbit::BETA0_BETA0:
		return "cgogn::GM_Orbit::BETA0_BETA1";
	case GM_Orbit::BETA0_BETA2:
		return "cgogn::GM_Orbit::BETA0_BETA2";
	case GM_Orbit::BETA1_BETA2:
		return "cgogn::GM_Orbit::BETA1_BETA2";
		
	case GM_Orbit::BETA0_BETA1_BETA2:
		return "cgogn::GM_Orbit::BETA0_BETA1_BETA2";
	case GM_Orbit::BETA0_BETA1_BETA3:
		return "cgogn::GM_Orbit::BETA0_BETA1_BETA3";
	case GM_Orbit::BETA0_BETA2_BETA3:
		return "cgogn::GM_Orbit::BETA0_BETA2_BETA3";		
	case GM_Orbit::BETA1_BETA2_BETA3:
		return "cgogn::GM_Orbit::BETA1_BETA2_BETA3";
	case GM_Orbit::BETA0_BETA1_BETA2_BETA3:
		return "cgogn::GM_Orbit::BETA0_BETA1_BETA2_BETA3";
		//		default: cgogn_assert_not_reached("This orbit does not exist"); return "UNKNOWN";
	}
	cgogn_assert_not_reached("This orbit does not exist");
#ifdef NDEBUG
	return "UNKNOWN"; // little trick to avoid warning on VS
#endif
}

/**
 * \brief Cellular typing
 * \tparam ORBIT The type of the orbit used to create the Cell
 */
// SAME AS CMAP
template <GM_Orbit ORBIT_>
struct Cell
{
	static const GM_Orbit ORBIT = ORBIT_;
	using Self = Cell<ORBIT>;

	/**
	 * \brief the dart representing this cell
	 */
	Dart dart;

	/**
	 * \brief Creates a new empty Cell as a nil dart.
	 */
	inline Cell() : dart()
	{
	}

	/**
	 * \brief Creates a new Cell with a dart.
	 * \param[in] d dart to convert to a cell of a given orbit
	 */
	inline explicit Cell(Dart d) : dart(d)
	{
	}

	/**
	 * \brief Copy constructor.
	 * Creates a new Cell from an another one.
	 * \param[in] c a cell
	 */
	inline Cell(const Self& c) : dart(c.dart)
	{
	}

	/**
	 * \brief Tests the validity of the cell.
	 * \retval true if the cell is valid
	 * \retval false otherwise
	 */
	inline bool is_valid() const
	{
		return !dart.is_nil();
	}

	/**
	 * \brief Assigns to the left hand side cell the value
	 * of the right hand side cell.
	 * \param[in] rhs the cell to assign
	 * \return The cell with the assigned value
	 */
	inline Self& operator=(Self rhs)
	{
		dart = rhs.dart;
		return *this;
	}

	/**
	 * \brief Prints a cell to a stream.
	 * \param[out] out the stream to print on
	 * \param[in] rhs the cell to print
	 */
	inline friend std::ostream& operator<<(std::ostream& out, const Self& rhs)
	{
		return out << rhs.dart;
	}

	/**
	 * \brief Reads a cell from a stream.
	 * \param[in] in the stream to read from
	 * \param[out] rhs the cell read
	 */
	inline friend std::istream& operator>>(std::istream& in, Self& rhs)
	{
		in >> rhs.dart;
		return in;
	}
};

}

} // namespace cgogn

#endif // CGOGN_CORE_TYPES_GMAP_CELL_H_
