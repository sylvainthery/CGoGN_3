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

#ifndef CGOGN_CORE_INCIDENCE_GRAPH_H_
#define CGOGN_CORE_INCIDENCE_GRAPH_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/container/attribute_container.h>
#include <cgogn/core/types/container/chunk_array.h>
#include <cgogn/core/types/container/vector.h>
#include <cgogn/core/types/mesh_traits.h>
#include <cgogn/core/utils/type_traits.h>
#include <cgogn/core/utils/tuples.h>
#include <cgogn/core/utils/thread_pool.h>


#include <any>
#include <array>

namespace cgogn
{

struct CGOGN_CORE_EXPORT IncidenceGraph
{
	// using AttributeContainer = AttributeContainerT<Vector>;
	using AttributeContainer = AttributeContainerT<ChunkArray>;

	template <typename T>
	using Attribute = AttributeContainer::Attribute<T>;
	using AttributeGen = AttributeContainer::AttributeGen;
	using MarkAttribute = AttributeContainer::MarkAttribute;

	/*************************************************************************/
	// Graph attributes container
	/*************************************************************************/

	// std::unordered_map<std::string, std::any> attributes_;

	struct Vertex
	{
		static const uint32 CELL_INDEX = 0;
		uint32 index_;
		inline Vertex() : index_(INVALID_INDEX)
		{
		}
		inline Vertex(uint32 id) : index_(id)
		{
		}
		bool operator<(Vertex v) const
		{
			return index_ < v.index_;
		}
		bool operator==(Vertex v) const
		{
			return index_ == v.index_;
		}
		inline bool is_valid() const
		{
			return index_ != INVALID_INDEX;
		}
	};

	struct Edge
	{
		static const uint32 CELL_INDEX = 1;
		uint32 index_;
		inline Edge() : index_(INVALID_INDEX)
		{
		}
		inline Edge(uint32 id) : index_(id)
		{
		}
		bool operator<(Edge e) const
		{
			return index_ < e.index_;
		}
		bool operator==(Edge e) const
		{
			return index_ == e.index_;
		}
		inline bool is_valid() const
		{
			return index_ != INVALID_INDEX;
		}
	};

	struct Face
	{
		static const uint32 CELL_INDEX = 2;
		uint32 index_;
		inline Face() : index_(INVALID_INDEX)
		{
		}
		inline Face(uint32 id) : index_(id)
		{
		}
		bool operator<(Face f) const
		{
			return index_ < f.index_;
		}
		bool operator==(Face f) const
		{
			return index_ == f.index_;
		}
		inline bool is_valid() const
		{
			return index_ != INVALID_INDEX;
		}
	};

	mutable std::array<AttributeContainer, 3> attribute_containers_;

	std::shared_ptr<Attribute<std::vector<Edge>>> vertex_incident_edges_;
	std::shared_ptr<Attribute<std::pair<Vertex, Vertex>>> edge_incident_vertices_;
	std::shared_ptr<Attribute<std::vector<Face>>> edge_incident_faces_;
	std::shared_ptr<Attribute<std::vector<Edge>>> face_incident_edges_;

	IncidenceGraph()
	{
		vertex_incident_edges_ =
			attribute_containers_[Vertex::CELL_INDEX].add_attribute<std::vector<Edge>>("incident_edges");
		edge_incident_vertices_ =
			attribute_containers_[Edge::CELL_INDEX].add_attribute<std::pair<Vertex, Vertex>>("incident_vertices");
		edge_incident_faces_ =
			attribute_containers_[Edge::CELL_INDEX].add_attribute<std::vector<Face>>("incident_faces");
		face_incident_edges_ =
			attribute_containers_[Face::CELL_INDEX].add_attribute<std::vector<Edge>>("incident_edges");
	};
	// ~IncidenceGraph();
};

template <>
struct mesh_traits<IncidenceGraph>
{
	static constexpr const char* name = "IncidenceGraph";
	static constexpr const uint8 dimension = 2;

	using Vertex = IncidenceGraph::Vertex;
	using Edge = IncidenceGraph::Edge;
	using Face = IncidenceGraph::Face;

	using Cells = std::tuple<Vertex, Edge, Face>;
	static constexpr const char* cell_names[] = {"Vertex", "Edge", "Face"};

	template <typename T>
	using Attribute = IncidenceGraph::Attribute<T>;
	using AttributeGen = IncidenceGraph::AttributeGen;
	using MarkAttribute = IncidenceGraph::MarkAttribute;
};

template <typename FUNC>
auto foreach_cell(const IncidenceGraph& ig, const FUNC& f)
{
    using CELL = func_parameter_type<FUNC>;
    for (uint32 i = ig.attribute_containers_[CELL::CELL_INDEX].first_index(),
                end = ig.attribute_containers_[CELL::CELL_INDEX].last_index();
         i != end; i = ig.attribute_containers_[CELL::CELL_INDEX].next_index(i))
    {
        CELL c(i);
        if (/*c.is_valid() && */ !f(c))
            break;
    }
}


template <typename FUNC>
auto parallel_foreach_cell(const IncidenceGraph& m, const FUNC& f)
{
    using CELL = func_parameter_type<FUNC>;
    static_assert(is_in_tuple<CELL, typename mesh_traits<IncidenceGraph>::Cells>::value,
                  "CELL not supported in this MESH");
    static_assert(is_func_parameter_same<FUNC, CELL>::value, "Wrong function cell parameter type");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

    ThreadPool* pool = thread_pool();
    uint32 nb_workers = pool->nb_workers();
    if (nb_workers == 0)
        return foreach_cell(m, f);

    using VecCell = std::vector<uint32>;
    using Future = std::future<void>;

    std::array<std::vector<VecCell*>, 2> cells_buffers;
    std::array<std::vector<Future>, 2> futures;
    cells_buffers[0].reserve(nb_workers);
    cells_buffers[1].reserve(nb_workers);
    futures[0].reserve(nb_workers);
    futures[1].reserve(nb_workers);

    Buffers<uint32>* buffers = uint32_buffers();

    uint32 it = m.attribute_containers_[CELL::CELL_INDEX].first_index();
    uint32 last = m.attribute_containers_[CELL::CELL_INDEX].last_index();

    uint32 i = 0u; // buffer id (0/1)
    uint32 j = 0u; // thread id (0..nb_workers)

    while (it < last)
    {
        // fill buffer
        cells_buffers[i].push_back(buffers->buffer());
        VecCell& cells = *cells_buffers[i].back();
        cells.reserve(PARALLEL_BUFFER_SIZE);
        for (uint32 k = 0u; k < PARALLEL_BUFFER_SIZE && it < last; ++k)
        {
            cells.push_back(it);
            it = m.attribute_containers_[CELL::CELL_INDEX].next_index(it);
        }
        // launch thread
        futures[i].push_back(pool->enqueue([&cells, &f]() {
            for (uint32 index : cells)
                f(CELL(index));
        }));
        // next thread
        if (++j == nb_workers)
        { // again from 0 & change buffer
            j = 0u;
            i = (i + 1u) % 2u;
            for (auto& fu : futures[i])
                fu.wait();
            for (auto& b : cells_buffers[i])
                buffers->release_buffer(b);
            futures[i].clear();
            cells_buffers[i].clear();
        }
    }

    // clean all at the end
    for (auto& fu : futures[0u])
        fu.wait();
    for (auto& b : cells_buffers[0u])
        buffers->release_buffer(b);
    for (auto& fu : futures[1u])
        fu.wait();
    for (auto& b : cells_buffers[1u])
        buffers->release_buffer(b);
}




/***********************************************
 *
 *      LOW_LEVEL OPERATOR
 *
 * **********************************************/



IncidenceGraph::Vertex CGOGN_CORE_EXPORT add_vertex(IncidenceGraph& ig);

void CGOGN_CORE_EXPORT remove_vertex(IncidenceGraph& ig, IncidenceGraph::Vertex v);


IncidenceGraph::Edge CGOGN_CORE_EXPORT add_edge(IncidenceGraph& ig, IncidenceGraph::Vertex v0,
                                                IncidenceGraph::Vertex v1);

void CGOGN_CORE_EXPORT remove_edge(IncidenceGraph& ig, IncidenceGraph::Edge e);

IncidenceGraph::Vertex CGOGN_CORE_EXPORT cut_edge(IncidenceGraph& ig, IncidenceGraph::Edge e, bool set_indices = true);

// returns a vector of removed edges (except e) in addition to the resulting vertex
std::pair<IncidenceGraph::Vertex, std::vector<IncidenceGraph::Edge>> collapse_edge(IncidenceGraph& ig,
                                                                                   IncidenceGraph::Edge e,
                                                                                   bool set_indices = true);


IncidenceGraph::Face CGOGN_CORE_EXPORT add_face(IncidenceGraph& ig, std::vector<IncidenceGraph::Edge>& edges);

IncidenceGraph::Edge CGOGN_CORE_EXPORT cut_face(IncidenceGraph& m, IncidenceGraph::Vertex v1,
                                                IncidenceGraph::Vertex v2);

void CGOGN_CORE_EXPORT remove_face(IncidenceGraph& ig, IncidenceGraph::Face f);

inline void copy(IncidenceGraph& /*dst*/, const IncidenceGraph& /*src*/)
{
    // TODO
}

template <typename CELL>
auto get_mark_attribute(const IncidenceGraph& ig)
{
    static_assert(is_in_tuple<CELL, typename mesh_traits<IncidenceGraph>::Cells>::value,
                  "CELL not supported in this MESH");
    return ig.attribute_containers_[CELL::CELL_INDEX].get_mark_attribute();
}

template <typename CELL>
void release_mark_attribute(const IncidenceGraph& ig, IncidenceGraph::MarkAttribute* attribute)
{
    return ig.attribute_containers_[CELL::CELL_INDEX].release_mark_attribute(attribute);
}

}

#include<cgogn/core/types/cell_marker.h>

namespace cgogn
{

template <typename CELL, typename FUNC>
auto foreach_incident_vertex(const IncidenceGraph& ig, CELL c, const FUNC& func)
{
    using Vertex = IncidenceGraph::Vertex;
    using Edge = IncidenceGraph::Edge;
    using Face = IncidenceGraph::Face;

    static_assert(is_in_tuple<CELL, mesh_traits<IncidenceGraph>::Cells>::value, "CELL not supported in this MESH");
    static_assert(is_func_parameter_same<FUNC, Vertex>::value, "Wrong function cell parameter type");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

    if constexpr (std::is_same_v<CELL, Edge>)
    {
        std::pair<Vertex, Vertex>& evs = (*ig.edge_incident_vertices_)[c.index_];
        if (func(evs.first))
            func(evs.second);
    }
    else if constexpr (std::is_same_v<CELL, Face>)
    {
        CellMarkerStore<IncidenceGraph, Vertex> marker(ig);
        for (auto& ep : (*ig.face_incident_edges_)[c.index_])
        {
            std::pair<Vertex, Vertex>& evs = (*ig.edge_incident_vertices_)[ep.index_];
            bool stop = false;
            if (!marker.is_marked(evs.first))
            {
                marker.mark(evs.first);
                stop = !func(evs.first);
            }
            if (!marker.is_marked(evs.second) && !stop)
            {
                marker.mark(evs.second);
                stop = !func(evs.second);
            }
            if (stop)
                break;
        }
    }
}


template <typename CELL, typename FUNC>
auto foreach_incident_edge(const IncidenceGraph& ig, CELL c, const FUNC& func)
{
    using Edge = mesh_traits<IncidenceGraph>::Edge;

    static_assert(is_in_tuple<CELL, mesh_traits<IncidenceGraph>::Cells>::value,
                  "CELL not supported in this IncidenceGraph");
    static_assert(is_func_parameter_same<FUNC, Edge>::value, "Wrong function cell parameter type");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

    if constexpr (std::is_same_v<CELL, mesh_traits<IncidenceGraph>::Vertex>)
    {
        for (auto& ep : (*ig.vertex_incident_edges_)[c.index_])
        {
            if (!func(ep))
                break;
        }
    }
    else if constexpr (std::is_same_v<CELL, mesh_traits<IncidenceGraph>::Face>)
    {
        for (auto& ep : (*ig.face_incident_edges_)[c.index_])
        {
            if (!func(ep))
                break;
        }
    }
}

template <typename CELL, typename FUNC>
auto foreach_incident_face(const IncidenceGraph& ig, CELL c, const FUNC& func)
{
    using Face = mesh_traits<IncidenceGraph>::Face;

    static_assert(is_in_tuple<CELL, mesh_traits<IncidenceGraph>::Cells>::value,
                  "CELL not supported in this IncidenceGraph");
    static_assert(is_func_parameter_same<FUNC, Face>::value, "Wrong function cell parameter type");
    static_assert(is_func_return_same<FUNC, bool>::value, "Given function should return a bool");

    if constexpr (std::is_same_v<CELL, mesh_traits<IncidenceGraph>::Vertex>)
    {
        CellMarkerStore<IncidenceGraph, Face> marker(ig);
        for (auto& ep : (*ig.vertex_incident_edges_)[c.index_])
        {
            bool stop = false;
            for (auto& fp : (*ig.edge_incident_faces_)[ep.index_])
            {
                stop = !func(fp);
                if (stop)
                    break;
            }
            if (stop)
                break;
        }
    }
    else if constexpr (std::is_same_v<CELL, mesh_traits<IncidenceGraph>::Edge>)
    {
        for (auto& fp : (*ig.edge_incident_faces_)[c.index_])
        {
            if (!func(fp))
                break;
        }
    }
}


template <typename CELL>
bool is_indexed(const IncidenceGraph& /*m*/)
{
    return true;
}

template <typename CELL>
uint32 new_index(const IncidenceGraph& ig)
{
    uint32 id = ig.attribute_containers_[CELL::CELL_INDEX].new_index();
    // (*ig.cells_indices_[CELL::CELL_INDEX])[id] = id;
    return id;
}

template <typename CELL>
uint32 index_of(const IncidenceGraph& /*m*/, CELL c)
{
    return c.index_;
}


template <typename T, typename CELL, typename MESH,
          typename std::enable_if_t<std::is_convertible_v<MESH&, IncidenceGraph&>>* = nullptr>
std::shared_ptr<typename mesh_traits<MESH>::template Attribute<T>> add_attribute(MESH& m, const std::string& name)
{
    static_assert(is_in_tuple<CELL, typename mesh_traits<MESH>::Cells>::value, "CELL not supported in this MESH");
    IncidenceGraph& mb = static_cast<IncidenceGraph&>(m);
    return mb.attribute_containers_[CELL::CELL_INDEX].template add_attribute<T>(name);
}

template <typename T, typename CELL>
std::shared_ptr<IncidenceGraph::Attribute<T>> get_attribute(const IncidenceGraph& m, const std::string& name)
{
    static_assert(is_in_tuple<CELL, typename mesh_traits<IncidenceGraph>::Cells>::value,
                  "CELL not supported in this MESH");
    return m.attribute_containers_[CELL::CELL_INDEX].template get_attribute<T>(name);
}

template <typename CELL>
void remove_attribute(IncidenceGraph& m, const std::shared_ptr<IncidenceGraph::AttributeGen>& attribute)
{
    m.attribute_containers_[CELL::CELL_INDEX].remove_attribute(attribute);
}

template <typename CELL>
void remove_attribute(IncidenceGraph& m, IncidenceGraph::AttributeGen* attribute)
{
    m.attribute_containers_[CELL::CELL_INDEX].remove_attribute(attribute);
}

template <typename CELL, typename FUNC>
void foreach_attribute(const IncidenceGraph& m, const FUNC& f)
{
    using AttributeGen = IncidenceGraph::AttributeGen;
    static_assert(is_func_parameter_same<FUNC, const std::shared_ptr<AttributeGen>&>::value,
                  "Wrong function attribute parameter type");
    for (const std::shared_ptr<AttributeGen>& a : m.attribute_containers_[CELL::CELL_INDEX])
        f(a);
}

template <typename T, typename CELL, typename FUNC>
void foreach_attribute(const IncidenceGraph& m, const FUNC& f)
{
    using AttributeT = IncidenceGraph::Attribute<T>;
    using AttributeGen = IncidenceGraph::AttributeGen;
    static_assert(is_func_parameter_same<FUNC, const std::shared_ptr<AttributeT>&>::value,
                  "Wrong function attribute parameter type");
    for (const std::shared_ptr<AttributeGen>& a : m.attribute_containers_[CELL::CELL_INDEX])
    {
        std::shared_ptr<AttributeT> at = std::dynamic_pointer_cast<AttributeT>(a);
        if (at)
            f(at);
    }
}





} //end namespace cgogn



namespace cgogn
{

} // namespace cgogn


#endif // CGOGN_CORE_INCIDENCE_GRAPH_H_
