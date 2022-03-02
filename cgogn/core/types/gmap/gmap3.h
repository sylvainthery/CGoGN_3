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

#ifndef CGOGN_CORE_TYPES_GMAP_CMAP3_H_
#define CGOGN_CORE_TYPES_GMAP_CMAP3_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/gmap/gmap2.h>

namespace cgogn
{

struct CGOGN_CORE_EXPORT GMap3 : public GMap2
{
	static const uint8 dimension = 3;

	using Vertex = Cell<BETA1_BETA2_BETA_3>;
	using Vertex2 = Cell<BETA1_BETA2>;
	using HalfEdge = Cell<BETA0>;
	using Edge = Cell<BETA0_BETA2_BETA3>;
	using Edge2 = Cell<BETA0_BETA2>;
	using Face = Cell<BETA0_BETA_BETA3>;
	using Face2 = Cell<BETA0_BETA1>;
	using Volume = Cell<BETA0_BETA1_BETA2>;
	using CC = Cell<BETA0_BETA1_BETA2_BETA3>;
	using Cells = std::tuple<Vertex, Vertex2, HalfEdge, Edge, Edge2, Face, Face2, Volume>;

	std::shared_ptr<Attribute<Dart>> beta3_;

	GMap3() : GMap2()
	{
		phi3_ = add_relation("beta3");
	}
};

template <>
struct mesh_traits<GMap3>
{
	static constexpr const char* name = "GMap3";
	static constexpr const uint8 dimension = 3;

	using Vertex = GMap3::Vertex;
	using Vertex2 = GMap3::Vertex2;
	using HalfEdge = GMap3::HalfEdge;
	using Edge = GMap3::Edge;
	using Edge2 = GMap3::Edge2;
	using Face = GMap3::Face;
	using Face2 = GMap3::Face2;
	using Volume = GMap3::Volume;

	using Cells = std::tuple<Vertex, Vertex2, HalfEdge, Edge, Edge2, Face, Face2, Volume>;
	static constexpr const char* cell_names[] = {"Vertex", "Vertex2", "HalfEdge", "Edge",
												 "Edge2",  "Face",	  "Face2",	  "Volume"};

	template <typename T>
	using Attribute = GMapBase::Attribute<T>;
	using AttributeGen = GMapBase::AttributeGen;
	using MarkAttribute = GMapBase::MarkAttribute;
};


GMap3::Vertex CGOGN_CORE_EXPORT cut_edge(GMap3& m, GMap3::Edge e, bool set_indices = true);

} // namespace cgogn

#endif // CGOGN_CORE_TYPES_GMAP_CMAP3_H_
