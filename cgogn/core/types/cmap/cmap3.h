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

#ifndef CGOGN_CORE_TYPES_CMAP_CMAP3_H_
#define CGOGN_CORE_TYPES_CMAP_CMAP3_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/cmap/cmap2.h>
#include <memory>
namespace cgogn
{
::std::shared_ptr<int> p_int;

struct CGOGN_CORE_EXPORT CMap3 : public CMap2
{
	static const uint8 dimension = 3;

	using Vertex = Cell<PHI21_PHI31>;
	using Vertex2 = Cell<PHI21>;
	using HalfEdge = Cell<DART>;
	using Edge = Cell<PHI2_PHI3>;
	using Edge2 = Cell<PHI2>;
	using Face = Cell<PHI1_PHI3>;
	using Face2 = Cell<PHI1>;
	using Volume = Cell<PHI1_PHI2>;

    using Cells = ::std::tuple<Vertex, Vertex2, HalfEdge, Edge, Edge2, Face, Face2, Volume>;

    ::std::shared_ptr<Attribute<Dart>> phi3_;

    inline CMap3() : CMap2()
	{
        phi3_ = add_relation("phi3");
	}
};

template <>
struct mesh_traits<CMap3>
{
	static constexpr const char* name = "CMap3";
	static constexpr const uint8 dimension = 3;

	using Vertex = CMap3::Vertex;
	using Vertex2 = CMap3::Vertex2;
	using HalfEdge = CMap3::HalfEdge;
	using Edge = CMap3::Edge;
	using Edge2 = CMap3::Edge2;
	using Face = CMap3::Face;
	using Face2 = CMap3::Face2;
	using Volume = CMap3::Volume;

	using Cells = std::tuple<Vertex, Vertex2, HalfEdge, Edge, Edge2, Face, Face2, Volume>;
	static constexpr const char* cell_names[] = {"Vertex", "Vertex2", "HalfEdge", "Edge",
												 "Edge2",  "Face",	  "Face2",	  "Volume"};

	template <typename T>
	using Attribute = CMapBase::Attribute<T>;
	using AttributeGen = CMapBase::AttributeGen;
	using MarkAttribute = CMapBase::MarkAttribute;
};

inline Dart phi3(const CMap3& m, Dart d)
{
	return (*(m.phi3_))[d.index];
}

inline void phi3_sew(CMap3& m, Dart d, Dart e)
{
	cgogn_assert(phi3(m, d) == d);
	cgogn_assert(phi3(m, e) == e);
	(*(m.phi3_))[d.index] = e;
	(*(m.phi3_))[e.index] = d;
}

inline void phi3_unsew(CMap3& m, Dart d)
{
	Dart e = phi3(m, d);
	(*(m.phi3_))[d.index] = d;
	(*(m.phi3_))[e.index] = e;
}



template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI1_PHI3(const MESH& m, Dart d, const FUNC& f)
    -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
    foreach_dart_of_PHI1(m, d, [&](Dart fd) -> bool {
        if (f(fd))
            return f(phi3(m, fd));
        return false;
    });
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI2_PHI3(const MESH& m, Dart d, const FUNC& f)
    -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");
    Dart it = d;
    do
    {
        if (!f(it))
            break;
        it = phi2(m, it);
        if (!f(it))
            break;
        it = phi3(m, it);
    } while (it != d);
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI21_PHI31(const MESH& m, Dart d, const FUNC& f)
    -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

    DartMarkerStore<MESH> marker(m);
    const std::vector<Dart>& marked_darts = marker.marked_darts();

    marker.mark(d);
    for (uint32 i = 0; i < uint32(marked_darts.size()); ++i)
    {
        const Dart curr_dart = marked_darts[i];
        //			if ( !(is_boundary(curr_dart) && is_boundary(phi3(curr_dart))) )
        if (!f(curr_dart))
            break;

        const Dart d_1 = phi_1(m, curr_dart);
        const Dart d2_1 = phi2(m, d_1); // turn in volume
        const Dart d3_1 = phi3(m, d_1); // change volume

        if (!marker.is_marked(d2_1))
            marker.mark(d2_1);
        if (!marker.is_marked(d3_1))
            marker.mark(d3_1);
    }
}

template <typename MESH, typename FUNC>
auto foreach_dart_of_PHI1_PHI2_PHI3(const MESH& m, Dart d, const FUNC& f)
    -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
{
    static_assert(is_func_parameter_same<FUNC, Dart>::value, "Given function should take a Dart as parameter");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

    DartMarkerStore<MESH> marker(m);

    std::vector<Dart> visited_face2;
    visited_face2.push_back(d); // Start with the face of d

    // For every face added to the list
    for (uint32 i = 0; i < visited_face2.size(); ++i)
    {
        const Dart e = visited_face2[i];
        if (!marker.is_marked(e)) // Face2 has not been visited yet
        {
            // mark visited darts (current face2)
            // and add non visited phi2-adjacent face2 to the list of face2
            Dart it = e;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    do
            {
                if (!f(it)) // apply the function to the darts of the face2
                    return;
                marker.mark(it);			   // Mark
                const Dart adj2 = phi2(m, it); // Get phi2-adjacent face2
                if (!marker.is_marked(adj2))
                    visited_face2.push_back(adj2); // Add it
                it = phi1(m, it);
            } while (it != e);
            // add phi3-adjacent face2 to the list
            visited_face2.push_back(phi3(m, it));
        }
    }
}


CMap3::Vertex CGOGN_CORE_EXPORT cut_edge(CMap3& m, CMap3::Edge e, bool set_indices = true);

CMap3::Edge CGOGN_CORE_EXPORT cut_face(CMap3& m, CMap3::Vertex v1, CMap3::Vertex v2, bool set_indices = true);

CMap3::Face cut_volume(CMap3& m, const std::vector<Dart>& path, bool set_indices = true);

CMap3::Volume close_hole(CMap3& m, Dart d, bool set_indices = true);

uint32 close(CMap3& m, bool set_indices = true);

bool check_integrity(CMap3& m, bool verbose = true);

} // namespace cgogn

#endif // CGOGN_CORE_TYPES_CMAP_CMAP3_H_
