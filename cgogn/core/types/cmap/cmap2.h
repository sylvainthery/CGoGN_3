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

#ifndef CGOGN_CORE_TYPES_CMAP_CMAP2_H_
#define CGOGN_CORE_TYPES_CMAP_CMAP2_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/cmap/cmap1.h>

namespace cgogn
{

struct CGOGN_CORE_EXPORT CMap2 : public CMap1
{
	static const uint8 dimension = 2;

	using Vertex = Cell<PHI21>;
	using HalfEdge = Cell<DART>;
	using Edge = Cell<PHI2>;
	using Face = Cell<PHI1>;
	using Volume = Cell<PHI1_PHI2>;
	using CC = Volume;

	using Cells = std::tuple<Vertex, HalfEdge, Edge, Face, Volume>;

	std::shared_ptr<Attribute<Dart>> phi2_;

	CMap2() : CMap1()
	{
		phi2_ = add_relation("phi2");
	}
};

template <>
struct mesh_traits<CMap2>
{
	static constexpr const char* name = "CMap2";
	static constexpr const uint8 dimension = 2;

	using Vertex = CMap2::Vertex;
	using HalfEdge = CMap2::HalfEdge;
	using Edge = CMap2::Edge;
	using Face = CMap2::Face;
	using Volume = CMap2::Volume;

	using Cells = std::tuple<Vertex, HalfEdge, Edge, Face, Volume>;
	static constexpr const char* cell_names[] = {"Vertex", "HalfEdge", "Edge", "Face", "Volume"};

	template <typename T>
	using Attribute = CMapBase::Attribute<T>;
	using AttributeGen = CMapBase::AttributeGen;
	using MarkAttribute = CMapBase::MarkAttribute;
};


inline Dart phi2(const CMap2& m, Dart d)
{
	return (*(m.phi2_))[d.index];
}

inline void phi2_sew(CMap2& m, Dart d, Dart e)
{
	cgogn_assert(phi2(m, d) == d);
	cgogn_assert(phi2(m, e) == e);
	(*(m.phi2_))[d.index] = e;
	(*(m.phi2_))[e.index] = d;
}

inline void phi2_unsew(CMap2& m, Dart d)
{
	Dart e = phi2(m, d);
	(*(m.phi2_))[d.index] = d;
	(*(m.phi2_))[e.index] = e;
}



/*****************************************************************************/
// orbits traversals
/*****************************************************************************/

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI2(const MESH& m, Dart d, const FUNC& f)
    -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
    if (f(d))
        f(phi2(m, d));
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI21(const MESH& m, Dart d, const FUNC& f)
    -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
    Dart it = d;
    do
    {
        if (!f(it))
            break;
//        it = phi<-1, 2>(m, it);
          it = phi2(m,phi_1(m, it));
    } while (it != d);
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI1_PHI2(const MESH& m, Dart d, const FUNC& f)
    -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

    DartMarkerStore<MESH> marker(m);

    std::vector<Dart> visited_faces;
    visited_faces.push_back(d); // Start with the face of d

    // For every face added to the list
    for (uint32 i = 0; i < uint32(visited_faces.size()); ++i)
    {
        const Dart e = visited_faces[i];
        if (!marker.is_marked(e)) // Face has not been visited yet
        {
            // mark visited darts (current face)
            // and add non visited adjacent faces to the list of face
            Dart it = e;
            do
            {
                if (!f(it)) // apply the function to the darts of the face
                    return;
                marker.mark(it);			  // Mark
                const Dart adj = phi2(m, it); // Get adjacent face
                if (!marker.is_marked(adj))
                    visited_faces.push_back(adj); // Add it
                it = phi1(m, it);
            } while (it != e);
        }
    }
}

template <typename MESH, typename CELL, typename FUNC>
auto foreach_incident_halfedge(const MESH& m, CELL c, const FUNC& func)
    -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    using HalfEdge = typename mesh_traits<MESH>::HalfEdge;

    static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
    static_assert(is_func_parameter_same<FUNC, HalfEdge>::value, "Wrong function cell parameter type");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

    foreach_dart_of_orbit(m, c, [&](Dart d) -> bool { return func(HalfEdge(d)); });
}


CMap2::Vertex CGOGN_CORE_EXPORT cut_edge(CMap2& m, CMap2::Edge e, bool set_indices = true);

CMap2::Vertex CGOGN_CORE_EXPORT collapse_edge(CMap2& m, CMap2::Edge e, bool set_indices = true);

bool CGOGN_CORE_EXPORT flip_edge(CMap2& m, CMap2::Edge e, bool set_indices = true);

CMap2::Face CGOGN_CORE_EXPORT add_face(CMap2& m, uint32 size, bool set_indices = true);

void CGOGN_CORE_EXPORT merge_incident_faces(CMap2& m, CMap2::Edge e, bool set_indices = true);

CMap2::Edge CGOGN_CORE_EXPORT cut_face(CMap2& m, CMap2::Vertex v1, CMap2::Vertex v2, bool set_indices = true);

CMap2::Face close_hole(CMap2& m, Dart d, bool set_indices = true);

uint32 CGOGN_CORE_EXPORT close(CMap2& m, bool set_indices = true);

void CGOGN_CORE_EXPORT reverse_orientation(CMap2& m);

CMap2::Volume CGOGN_CORE_EXPORT add_pyramid(CMap2& m, uint32 size, bool set_indices = true);

CMap2::Volume CGOGN_CORE_EXPORT add_prism(CMap2& m, uint32 size, bool set_indices = true);

void CGOGN_CORE_EXPORT remove_volume(CMap2& m, CMap2::Volume v);

bool CGOGN_CORE_EXPORT check_integrity(CMap2& m, bool verbose);

bool CGOGN_CORE_EXPORT edge_can_collapse(const CMap2& m, CMap2::Edge e);

bool CGOGN_CORE_EXPORT edge_can_flip(const CMap2& m, CMap2::Edge e);

CMap2::Volume CGOGN_CORE_EXPORT add_pyramid(CMap2& m, uint32 size, bool set_indices);

CMap2::Volume CGOGN_CORE_EXPORT add_prism(CMap2& m, uint32 size, bool set_indices);

void CGOGN_CORE_EXPORT remove_volume(CMap2& m, CMap2::Volume v);

} // namespace cgogn

#endif // CGOGN_CORE_TYPES_CMAP_CMAP2_H_
