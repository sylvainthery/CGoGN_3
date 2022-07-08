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
#include <cgogn/core/types/map/cell.h>

#include <any>
#include <array>
#include <unordered_map>
#include <sstream>

namespace cgogn
{

struct CGOGN_CORE_EXPORT MapBase
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
	// shortcuts to embedded cells 
	std::vector<uint32> cells_embedded_orbit_;


	/*************************************************************************/
	// Cells attributes containers
	/*************************************************************************/
	mutable std::array<std::shared_ptr<AttributeContainer>, NB_ORBITS> attribute_containers_;

	MapBase();
	~MapBase();

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
        ///
        /// \brief begin
        /// \return
        ///
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

	inline MapBase* get_base_ptr()
	{
		return this;
	}

	inline const MapBase* get_base_ptr() const
	{
		return this;
	}

};



///
/// \brief is_boundary
/// \param m const ref MapBase inherited class
/// \param d dart to check
/// \return does this dart belong to the boundary
///

//inline bool is_boundary(const MapBase& m, Dart d)
//{
//	return (*m.boundary_marker_)[d.index] != 0u;
//}

///
/// \brief set_boundary
/// \param m ref MapBase inherited class
/// \param d
/// \param b
///
//inline void set_boundary(const MapBase& m, Dart d, bool b) //TODO: const ??
//{
//	(*m.boundary_marker_)[d.index] = b ? 1u : 0u;
//}

///
/// \brief nb_darts
/// \param m
/// \return number of darts of the map, O(0).
///
inline uint32 nb_darts(const MapBase& m)
{
	return m.darts_.nb_elements();
}

///
/// \brief dump_map_darts low level dump for debugging
/// \param m
///
//void dump_map_darts(const MapBase& m);

///
/// \brief add_dart [LOW LEVEL] Add a dart to a map. A dart is only an encapsulated index
/// \param m
/// \return itself
///
Dart add_dart(MapBase& m);

///
/// \brief remove_dart [LOW LEVEL] Remove a dart from the map
/// \param m
/// \param d
///
void remove_dart(MapBase& m, Dart d);


///
/// \brief set_index  [LOW LEVEL] Set index of CELL embedding (in table of embedding) of a dart
/// \param m
/// \param d
/// \param index
///
template <typename CELL>
void set_index(MapBase& m, Dart d, uint32 index)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	//	constexpr int32 obv = Orbit_Value(orbit);
	//	static_assert(obv < NB_ORBITS, "Unknown orbit parameter");
	const int32 obv = orbit;
	const uint32 old = (*m.cells_indices_[orbit])[d.index];
	// ref_index() is done before unref_index() to avoid deleting the index if old == index
	if (index != INVALID_INDEX)
		m.attribute_containers_[obv]->ref_index(index); // ref the new index
	if (old != INVALID_INDEX)
		m.attribute_containers_[obv]->unref_index(old); // unref the old index
	(*m.cells_indices_[orbit])[d.index] = index;		 // affect the index to the dart
}


void clear(MapBase& m, bool keep_attributes = true);

///
/// \brief is_indexed Is this kind of CELL of map p embedded ?
/// \param m
/// \return
///
template <typename CELL>
bool is_indexed(const MapBase& m)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	return m.cells_indices_[orbit] != nullptr;
}

///
/// \brief is_indexed Is this orbit of map p embedded ?
/// \param m
/// \param orbit
/// \return
///
inline bool is_indexed(const MapBase& m, Orbit orbit)
{
	cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	return m.cells_indices_[orbit] != nullptr;
}

template <typename CELL>
uint32 maximum_index(const MapBase& m)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	cgogn_message_assert(is_indexed<CELL>(m), "Trying to access a cell index of an unindexed cell type");
	return m.attribute_containers_[CELL::ORBIT]->maximum_index();
}

///
/// \brief index_of return index of a cell o a map
/// \param m
/// \param c
/// \return
///
template <typename CELL>
uint32 index_of(const MapBase& m, CELL c)
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
	return (*m.cells_indices_[orbit])[c.dart.index];
}

///
/// \brief of_index Get the cell of type CELL of a given index
/// \param m
/// \param i
/// \return
///


///
/// \brief new_index Reserve a new line in CELL attribute container
/// \param m
/// \return
///
template <typename CELL>
uint32 new_index(const MapBase& m)
{
	return m.attribute_containers_[CELL::ORBIT]->new_index();
}

/////
///// \brief init_cells_indexing Add an index attribute on dart for orbit embedding
///// \param m
///// \param orbit
/////
//inline void init_cells_indexing(MapBase& m, Orbit orbit)
//{
//	cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
//	if (!is_indexed(m, orbit))
//	{
//		std::ostringstream oss;
//		oss << "__index_" << orbit_name(orbit);
//		m.cells_indices_[orbit] = m.darts_.add_attribute<uint32>(oss.str());
//		m.cells_indices_[orbit]->fill(INVALID_INDEX);
//	}
//}


template <typename CELL>
void remove_attribute(MapBase& m, const std::shared_ptr<MapBase::AttributeGen>& attribute)
{
	m.attribute_containers_[CELL::ORBIT]->remove_attribute(attribute);
}

template <typename CELL>
void remove_attribute(MapBase& m, MapBase::AttributeGen* attribute)
{
	m.attribute_containers_[CELL::ORBIT]->remove_attribute(attribute);
}


///
/// \brief get_dart_mark_attribute [LOW LEVEL]
/// \param m
/// \return a pointer on Attribute reserved for DartMarkers
///
inline typename MapBase::MarkAttribute* get_dart_mark_attribute(const MapBase& m)
{
	return m.darts_.get_mark_attribute();
}

///
/// \brief release_dart_mark_attribute [LOW LEVEL] ro release a marker on dart attribute reserved by get_dart_mark_attribute
/// \param m
/// \param attribute pointer on the reserved attribute
///
inline void release_dart_mark_attribute(const MapBase& m, MapBase::MarkAttribute* attribute)
{
	return m.darts_.release_mark_attribute(attribute);
}

///
/// \brief release_mark_attribute
/// \param m
/// \param attribute
///
template <typename CELL>
void release_mark_attribute(const MapBase& m, MapBase::MarkAttribute* attribute)
{
	return m.attribute_containers_[CELL::ORBIT]->release_mark_attribute(attribute);
}


template <typename CELL, typename FUNC>
void foreach_attribute(const MapBase& m, const FUNC& f)
{
	using AttributeGen = MapBase::AttributeGen;
	static_assert(is_func_parameter_same<FUNC, const std::shared_ptr<AttributeGen>&>::value,
				  "Wrong function attribute parameter type");
	for (const std::shared_ptr<AttributeGen>& a : *(m.attribute_containers_[CELL::ORBIT]))
		f(a);
}

template <typename T, typename CELL, typename FUNC>
void foreach_attribute(const MapBase& m, const FUNC& f)
{
	using AttributeT = MapBase::Attribute<T>;
	using AttributeGen = MapBase::AttributeGen;
	static_assert(is_func_parameter_same<FUNC, const std::shared_ptr<AttributeT>&>::value,
				  "Wrong function attribute parameter type");
	for (const std::shared_ptr<AttributeGen> a : *(m.attribute_containers_[CELL::ORBIT]))
	{
		std::shared_ptr<AttributeT> at = std::dynamic_pointer_cast<AttributeT>(a);
		if (at)
			f(at);
	}
}

template <typename T>
T& get_attribute(MapBase& m, const std::string& name)
{
	return m.get_attribute<T>(name);
}


template <typename CELL>
void copy_index(MapBase& m, Dart dest, Dart src) 
{
	static const Orbit orbit = CELL::ORBIT;
	static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
	set_index<CELL>(m, dest, index_of(m, CELL(src)));
}

} // namespace cgogn

#endif // CGOGN_CORE_CMAP_CMAP_BASE_H_
