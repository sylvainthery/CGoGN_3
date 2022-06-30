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

#ifndef CGOGN_CORE_TYPES_GMAP_GMAP0_H_
#define CGOGN_CORE_TYPES_GMAP_GMAP0_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/map/gmap/gmap_base.h>
#include <cgogn/core/types/map/cell.h>

namespace cgogn
{

struct CGOGN_CORE_EXPORT GMap0 : public GMapBase
{
	static const uint8 dimension = 0;

	using Vertex = Cell < Orbit::DART > ;

	using Cells = std::tuple<Vertex>;

	std::shared_ptr<Attribute<Dart>> beta0_;

	GMap0()
	{
		beta0_ = add_relation("beta0");
	}

};

template <>
struct mesh_traits<GMap0>
{
	using MeshType = GMap0;
	static constexpr const char* name = "GMap0";
	static constexpr const uint8 dimension = 0;

	using Vertex = typename GMap0::Vertex;

	using Cells = std::tuple<Vertex>;
	static constexpr const char* cell_names[] = {"Vertex"};

	template <typename T>
	using Attribute = GMapBase::Attribute<T>;
	using AttributeGen = GMapBase::AttributeGen;
	using MarkAttribute = GMapBase::MarkAttribute;
};

inline Dart beta0(const GMap0& m, Dart d)
{
	return (*(m.beta0_))[d.index];
}

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


template <uint8 Arg, uint8... Args, typename MESH>
inline Dart beta(const MESH& m, Dart d)
{
	static_assert((Arg >= 0 && Arg <= mesh_traits<MESH>::dimension), "Bad beta value");

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
}

} // namespace cgogn

#endif // CGOGN_CORE_TYPES_GMAP_GMAP0_H_
