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

#ifndef CGOGN_CORE_TYPES_GMAP_PHI_H_
#define CGOGN_CORE_TYPES_GMAP_PHI_H_

#include <cgogn/core/types/GMap/GMap3.h>

namespace cgogn
{

/*****************************************************************************/

// template <typename MESH>
// Dart phiX(const MESH& m, Dart d);

/*****************************************************************************/

//////////////
// GMapBase //
//////////////

inline Dart beta0(const GMap0& m, Dart d)
{
	return (*(m.beta0_))[d.index];
}

inline Dart beta1(const GMap1& m, Dart d)
{
	return (*(m.beta1_))[d.index];
}

inline Dart beta2(const GMap2& m, Dart d)
{
	return (*(m.beta2_))[d.index];
}

inline Dart beta3(const GMap3& m, Dart d)
{
	return (*(m.beta3_))[d.index];
}


template <int8 Arg, uint8... Args, typename MESH>
inline Dart beta(const MESH& m, Dart d)
{
	static_assert((Arg >= -1 && Arg <= mesh_traits<MESH>::dimension), "Bad beta value");

	Dart res;
	if constexpr (Arg == 0)
		res = beta0(m, d);
	if constexpr (Arg == 1)
		res = beta1(m, d);
	if constexpr (Arg == 2)
		res = beta2(m, d);
	if constexpr (Arg == 3)
		res = beta3(m, d);

	if constexpr (sizeof...(Args) > 0)
		return beta<Args...>(m, res);
	else
		return res;

	
//////////////
// GMapBase //
//////////////

inline void beta0_sew(GMap0& m, Dart d, Dart e)
{
	cgogn_assert(beta0(m, d) == d);
	cgogn_assert(beta0(m, e) == e);
	(*(m.beta0_))[d.index] = e;
	(*(m.beta0_))[e.index] = d;
}

inline void beta0_unsew(GMap0& m, Dart d, Dart e)
{
	(*(m.beta0_))[d.index] = d;
	(*(m.beta0_))[e.index] = e;
}

inline void beta1_sew(GMap1& m, Dart d, Dart e)
{
	cgogn_assert(beta1(m, d) == d);
	cgogn_assert(beta1(m, e) == e);
	(*(m.beta1_))[d.index] = e;
	(*(m.beta1_))[e.index] = d;
}

inline void beta1_unsew(GMap1& m, Dart d, Dart e)
{
	(*(m.beta1_))[d.index] = d;
	(*(m.beta1_))[e.index] = e;
}

inline void beta2_sew(GMap2& m, Dart d, Dart e)
{
	cgogn_assert(beta2(m, d) == d);
	cgogn_assert(beta2(m, e) == e);
	(*(m.beta2_))[d.index] = e;
	(*(m.beta2_))[e.index] = d;
}

inline void beta2_unsew(GMap2& m, Dart d, Dart e)
{
	(*(m.beta2_))[d.index] = d;
	(*(m.beta2_))[e.index] = e;
}

inline void beta3_sew(GMap3& m, Dart d, Dart e)
{
	cgogn_assert(beta3(m, d) == d);
	cgogn_assert(beta3(m, e) == e);
	(*(m.beta3_))[d.index] = e;
	(*(m.beta3_))[e.index] = d;
}

inline void beta3_unsew(GMap3& m, Dart d, Dart e)
{
	(*(m.beta3_))[d.index] = d;
	(*(m.beta3_))[e.index] = e;
}




} // namespace cgogn

#endif // CGOGN_CORE_TYPES_GMAP_PHI_H_
