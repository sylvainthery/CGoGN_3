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

#include <cgogn/core/types/cmap/cell.h>
#include <cgogn/core/utils/tuples.h>
#include <any>
#include <array>
#include <unordered_map>


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

	enum TraversalPolicy
	{
		AUTO,
		DART_MARKING
	};

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


    void clear(CMapBase& m, bool keep_attributes = true);

//    template <typename MESH, typename std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>* = nullptr>
//    void copy(MESH& dst, const MESH& src)

    void copy(CMapBase& dst, const CMapBase& src);


    inline bool is_indexed(const CMapBase& m, Orbit orbit)
    {
        cgogn_message_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
        return m.cells_indices_[orbit] != nullptr;
    }

    template <typename CELL>
    bool is_indexed(const CMapBase& m)
    {
        static const Orbit orbit = CELL::ORBIT;
        static_assert(orbit < NB_ORBITS, "Unknown orbit parameter");
        return m.cells_indices_[orbit] != nullptr;
    }

    template <typename CELL>
    uint32 new_index(const CMapBase& m)
    {
        return m.attribute_containers_[CELL::ORBIT].new_index();
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

    template <typename CELL, typename MESH>
    auto set_index(MESH& m, CELL c, uint32 index) -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
    {
        static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
        cgogn_message_assert(is_indexed<CELL>(m), "Trying to access the cell index of an unindexed cell type");
        foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
            set_index<CELL>(m, d, index);
            return true;
        });
    }

    template <typename CELL, typename MESH>
    auto index_cells(MESH& m) -> std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>
    {
        static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");
        if (!is_indexed<CELL>(m))
            init_cells_indexing<CELL>(m);

        CMapBase& base = static_cast<CMapBase&>(m);
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




    template <typename CELL, typename MESH, typename std::enable_if_t<std::is_convertible_v<MESH&, CMapBase&>>* = nullptr>
    bool check_indexing(MESH& m, bool verbose = true)
    {
        static_assert(is_in_tuple_v<CELL, typename mesh_traits<MESH>::Cells>, "CELL not supported in this MESH");

        if (!is_indexed<CELL>(m))
            return true;

        bool result = true;

        auto counter = add_attribute<uint32, CELL>(m, "__cell_counter");
        counter->fill(0);

        foreach_cell(
            m,
            [&](CELL c) -> bool {
                const uint32 index = index_of(m, c);

                ++(*counter)[index];

                bool valid_index = index != INVALID_INDEX;
                if (verbose && !valid_index)
                    std::cerr << "Cell " << c << " (" << cell_name<CELL>(m) << ") has invalid index" << std::endl;

                bool all_darts_same_index = true;
                foreach_dart_of_orbit(m, c, [&](Dart d) -> bool {
                    const uint32 index_d = index_of(m, CELL(d));
                    if (index_d != index)
                    {
                        if (verbose)
                            std::cerr << "Cell " << c << " (" << cell_name<CELL>(m) << ") has darts with different indices"
                                      << std::endl;
                        all_darts_same_index = false;
                    }
                    return true;
                });

                result &= valid_index && all_darts_same_index;
                return true;
            },
            CMapBase::TraversalPolicy::DART_MARKING);

        // check that all lines of the attribute container are used
        for (uint32 i = m.attribute_containers_[CELL::ORBIT].first_index(),
                    end = m.attribute_containers_[CELL::ORBIT].last_index();
             i != end; i = m.attribute_containers_[CELL::ORBIT].next_index(i))
        {
            if ((*counter)[i] == 0)
            {
                if (verbose)
                    std::cerr << "Cell index " << i << " is not used in container " << cell_name<CELL>(m) << std::endl;
                result = false;
            }
            else
            {
                if ((*counter)[i] >= 2ul)
                {
                    if (verbose)
                        std::cerr << "Multiple cells with same index " << i << " in container " << cell_name<CELL>(m)
                                  << std::endl;
                    result = false;
                }
            }
        }

        remove_attribute<CELL>(m, counter);

        return result;
    }



} // namespace cgogn

#endif // CGOGN_CORE_CMAP_CMAP_BASE_H_
