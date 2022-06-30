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

#ifndef CGOGN_CORE_TYPES_GMAP_GMAP1_H_
#define CGOGN_CORE_TYPES_GMAP_GMAP1_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/map/gmap/gmap0.h>

namespace cgogn
{

struct CGOGN_CORE_EXPORT GMap1 : public GMap0
{
	static const uint8 dimension = 1;

	using Vertex = Cell<Orbit::DART>;
	using Edge = Cell<Orbit::BETA0>;
	using Face = Cell<Orbit::BETA0_BETA1>;

	using Cells = std::tuple<Vertex, Edge, Face>;

	std::shared_ptr<Attribute<Dart>> beta1_;

	GMap1() : GMap0()
	{
		beta1_ = add_relation("beta1");
	}

};

template <>
struct mesh_traits<GMap1>
{
	using MeshType = GMap1;
	static constexpr const uint8 dimension = 1;

	using Vertex = GMap1::Vertex;
	using Edge = GMap1::Edge;
	using Face = GMap1::Face;

	using Cells = std::tuple<Vertex, Edge, Face>;
	static constexpr const char* cell_names[] = {"Vertex", "Edge", "Face"};

	template <typename T>
	using Attribute = GMapBase::Attribute<T>;
	using AttributeGen = GMapBase::AttributeGen;
	static constexpr const char* name = "GMap1";
using MarkAttribute = GMapBase::MarkAttribute;
};

GMap1::Vertex CGOGN_CORE_EXPORT cut_edge(GMap1& m, GMap1::Edge e, bool set_indices = true);

GMap1::Face CGOGN_CORE_EXPORT add_face(GMap1& m, uint32 size, bool set_indices = true);


inline Dart beta1(const GMap1& m, Dart d)
{
	return (*(m.beta1_))[d.index];
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


/*****************************************************************************/

// template <typename CMAP>
// void remove_dart(CMAP& m, Dart d);

/*****************************************************************************/


/*}****************************************************************************/

} // namespace cgogn

#endif // CGOGN_CORE_TYPES_GMAP_GMAP1_H_

