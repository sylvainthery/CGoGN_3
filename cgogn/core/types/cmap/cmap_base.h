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

#ifndef CGOGN_CORE_CMAP_CMAP_BASE_H_
#define CGOGN_CORE_CMAP_CMAP_BASE_H_

#include <cgogn/core/cgogn_core_export.h>

#include <cgogn/core/types/container/attribute_container.h>
#include <cgogn/core/types/container/chunk_array.h>
#include <cgogn/core/types/container/vector.h>
#include <cgogn/core/types/mesh_traits.h>
#include <cgogn/core/utils/type_traits.h>
#include <cgogn/core/types/cmap/cell.h>
#include <cgogn/core/types/cmap/dart_marker.h>

#include <any>
#include <array>
#include <unordered_map>
#include <sstream>


namespace cgogn
{

struct CGOGN_CORE_EXPORT CMapBase
{
	// using AttributeContainer = AttributeContainerT<Vector>;
	using AttributeContainer = AttributeContainerT<ChunkArray>;

	template <typename T>
	using Attribute = AttributeContainer::Attribute<T>;
	using AttributeGen = AttributeContainer::AttributeGen;
	using MarkAttribute = AttributeContainer::MarkAttribute;

	/*************************************************************************/
	// Map-wise attributes container
	/*************************************************************************/
	std::unordered_map<std::string, std::any> attributes_;

	/*************************************************************************/
	// Dart attributes container
	/*************************************************************************/
	mutable AttributeContainer darts_;

	// shortcuts to topological relations attributes
	std::vector<std::shared_ptr<Attribute<Dart>>> relations_;
	// shortcuts to cells indices attributes
	std::array<std::shared_ptr<Attribute<uint32>>, NB_ORBITS> cells_indices_;

	// shortcut to boundary marker attribute
	MarkAttribute* boundary_marker_;

	/*************************************************************************/
	// Cells attributes containers
	/*************************************************************************/
	mutable std::array<AttributeContainer, NB_ORBITS> attribute_containers_;

	//enum TraversalPolicy
	//{
	//	AUTO,
	//	DART_MARKING
	//};

	CMapBase();
	~CMapBase();

	// Map-wise attributes
	template <typename T>
	T& get_attribute(const std::string& name)
	{
		auto [it, inserted] = attributes_.try_emplace(name, T());
		return std::any_cast<T&>(it->second);
	}

	inline std::shared_ptr<Attribute<Dart>> add_relation(const std::string& name)
	{
		return relations_.emplace_back(darts_.add_attribute<Dart>(name));
	}

	inline Dart begin() const
	{
		return Dart(darts_.first_index());
	}
	inline Dart end() const
	{
		return Dart(darts_.last_index());
	}
	inline Dart next(Dart d) const
	{
		return Dart(darts_.next_index(d.index));
	}
};

template <typename T>
T& get_attribute(CMapBase& m, const std::string& name)
{
	return m.get_attribute<T>(name);
}



template <typename CELL>
void remove_attribute(CMapBase& m, const std::shared_ptr<CMapBase::AttributeGen>& attribute)
{
	m.attribute_containers_[CELL::ORBIT].remove_attribute(attribute);
}

template <typename CELL>
void remove_attribute(CMapBase& m, CMapBase::AttributeGen* attribute)
{
	m.attribute_containers_[CELL::ORBIT].remove_attribute(attribute);
}


template <typename CELL, typename FUNC>
void foreach_attribute(const CMapBase& m, const FUNC& f)
{
	using AttributeGen = CMapBase::AttributeGen;
	static_assert(is_func_parameter_same<FUNC, const std::shared_ptr<AttributeGen>&>::value,
				  "Wrong function attribute parameter type");
	for (const std::shared_ptr<AttributeGen>& a : m.attribute_containers_[CELL::ORBIT])
		f(a);
}

template <typename T, typename CELL, typename FUNC>
void foreach_attribute(const CMapBase& m, const FUNC& f)
{
	using AttributeT = CMapBase::Attribute<T>;
	using AttributeGen = CMapBase::AttributeGen;
	static_assert(is_func_parameter_same<FUNC, const std::shared_ptr<AttributeT>&>::value,
				  "Wrong function attribute parameter type");
	for (const std::shared_ptr<AttributeGen>& a : m.attribute_containers_[CELL::ORBIT])
	{
		std::shared_ptr<AttributeT> at = std::dynamic_pointer_cast<AttributeT>(a);
		if (at)
			f(at);
	}
}


inline Dart add_dart(CMapBase& m)
{
	uint32 index = m.darts_.new_index();
	Dart d(index);
	for (auto& rel : m.relations_)
		(*rel)[d.index] = d;
	for (auto& emb : m.cells_indices_)
		if (emb)
			(*emb)[d.index] = INVALID_INDEX;
	return d;
}

inline void remove_dart(CMapBase& m, Dart d)
{
	for (uint32 orbit = 0; orbit < NB_ORBITS; ++orbit)
	{
		if (m.cells_indices_[orbit])
		{
			uint32 index = (*m.cells_indices_[orbit])[d.index];
			if (index != INVALID_INDEX)
				m.attribute_containers_[orbit].unref_index(index);
		}
	}
	m.darts_.release_index(d.index);
}

inline void set_boundary(const CMapBase& m, Dart d, bool b)
{
	(*m.boundary_marker_)[d.index] = b ? 1u : 0u;
}


template <typename CELL>
void set_index(CMapBase& m, Dart d, uint32 index)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	const uint32 old = (*m.cells_indices_[orbit])[d.index];
	// ref_index() is done before unref_index() to avoid deleting the index if old == index
	if (index != INVALID_INDEX)
		m.attribute_containers_[orbit].ref_index(index); // ref the new index
	if (old != INVALID_INDEX)
		m.attribute_containers_[orbit].unref_index(old); // unref the old index
	(*m.cells_indices_[orbit])[d.index] = index;		 // affect the index to the dart
}

template <typename CELL>
void copy_index(CMapBase& m, Dart dest, Dart src)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	set_index<CELL>(m, dest, index_of(m, CELL(src)));
}


inline bool is_boundary(const CMapBase& m, Dart d)
{
	return (*m.boundary_marker_)[d.index] != 0u;
}

inline uint32 nb_darts(const CMapBase& m)
{
	return m.darts_.nb_elements();
}

void dump_map_darts(const CMapBase& m);

template <typename CELL>
bool is_indexed(const CMapBase& m)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	return m.cells_indices_[orbit] != nullptr;
}

inline bool is_indexed(const CMapBase& m, Orbit orbit)
{
	cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	return m.cells_indices_[orbit] != nullptr;
}

template <typename CELL>
uint32 maximum_index(const CMapBase& m)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	cgogn_message_assert(is_indexed<CELL>(m), "Trying to access a cell index of an unindexed cell type");
	return m.attribute_containers_[CELL::ORBIT].maximum_index();
}

template <typename CELL>
uint32 index_of(const CMapBase& m, CELL c)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
	return (*m.cells_indices_[orbit])[c.dart.index];
}

template <typename CELL>
CELL of_index(const CMapBase& m, uint32 i)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
	for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
	{
		if (!is_boundary(m, d))
		{
			const CELL c(d);
			if (index_of(m, c) == i)
				return c;
		}
	}
	return CELL();
}

template <typename CELL>
uint32 new_index(const CMapBase& m)
{
	return m.attribute_containers_[CELL::ORBIT].new_index();
}


template <typename CELL>
void init_cells_indexing(CMapBase& m)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	if (!is_indexed<CELL>(m))
	{
		std::ostringstream oss;
		oss << "__index_" << orbit_name(orbit);
		m.cells_indices_[orbit] = m.darts_.add_attribute<uint32>(oss.str());
		m.cells_indices_[orbit]->fill(INVALID_INDEX);
	}
}

inline void init_cells_indexing(CMapBase& m, Orbit orbit)
{
	cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	if (!is_indexed(m, orbit))
	{
		std::ostringstream oss;
		oss << "__index_" << orbit_name(orbit);
		m.cells_indices_[orbit] = m.darts_.add_attribute<uint32>(oss.str());
		m.cells_indices_[orbit]->fill(INVALID_INDEX);
	}
}

inline typename CMapBase::MarkAttribute* get_dart_mark_attribute(const CMapBase& m)
{
	return m.darts_.get_mark_attribute();
}

inline void release_dart_mark_attribute(const CMapBase& m, CMapBase::MarkAttribute* attribute)
{
	return m.darts_.release_mark_attribute(attribute);
}

template <typename CELL>
void release_mark_attribute(const CMapBase& m, CMapBase::MarkAttribute* attribute)
{
	return m.attribute_containers_[CELL::ORBIT].release_mark_attribute(attribute);
}


void clear(CMapBase& m, bool keep_attributes = true);

void copy(CMapBase& dst, const CMapBase& src);


template <typename CELL, typename MESH>
auto index_cells(MESH& m) -> std::enable_if_t<mesh_traits<MESH>::is_CMapBase>
{
    static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
    if (!is_indexed<CELL>(m))
        init_cells_indexing<CELL>(m);

    typename mesh_traits<MESH>::Base& base = static_cast<typename mesh_traits<MESH>::Base&>(m);
    DartMarker dm(m);
    for (Dart d = base.begin(), end = base.end(); d != end; d = base.next(d))
    {
        if (!is_boundary(m, d) && !dm.is_marked(d))
        {
            const CELL c(d);
            foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
                dm.mark(d);
                return true;
            });

            if (index_of(m, c) == INVALID_INDEX)
                set_index(m, c, new_index<CELL>(m));
        }
    }
}


} // namespace cgogn

#endif // CGOGN_CORE_CMAP_CMAP_BASE_H_
