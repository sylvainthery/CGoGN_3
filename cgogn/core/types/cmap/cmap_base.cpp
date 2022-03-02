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

#include <cgogn/core/types/cmap/cmap3.h>

namespace cgogn
{

CMapBase::CMapBase()
{
	boundary_marker_ = darts_.get_mark_attribute();
}

CMapBase::~CMapBase()
{
}


void clear(CMapBase& m, bool keep_attributes)
{
    // clear darts and keep attributes (phi relations)
    m.darts_.clear_attributes();
    if (!keep_attributes)
    {
        // remove cells indices attributes
        for (uint32 orbit = 0; orbit < NB_ORBITS; ++orbit)
        {
            if (m.cells_indices_[orbit] != nullptr)
            {
                m.darts_.remove_attribute(m.cells_indices_[orbit]);
                m.cells_indices_[orbit].reset();
            }
        }
    }

    // clear all cell attributes
    for (CMapBase::AttributeContainer& container : m.attribute_containers_)
    {
        if (keep_attributes)
            container.clear_attributes();
        else
        {
            container.clear_attributes();
            // if there are still shared_ptr somewhere, some attributes may not be removed
            container.remove_attributes();
        }
    }
}


void copy(CMapBase& dst, const CMapBase& src)
{
    clear(dst, false);
    for (uint32 orbit = 0; orbit < NB_ORBITS; ++orbit)
    {
        if (src.cells_indices_[orbit] != nullptr)
            init_cells_indexing(dst, Orbit(orbit));
    }
    dst.darts_.copy(src.darts_);
    for (uint32 i = 0; i < NB_ORBITS; ++i)
        dst.attribute_containers_[i].copy(src.attribute_containers_[i]);
    dst.boundary_marker_ = dst.darts_.get_mark_attribute();
    dst.boundary_marker_->copy(*src.boundary_marker_);
}

bool check_integrity(CMap1& m, bool verbose)
{
    bool result = true;
    for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
    {
        bool relations = phi<-1, 1>(m, d) == d && phi<1, -1>(m, d) == d;
        if (verbose && !relations)
            std::cerr << "Dart " << d << " has bad relations" << std::endl;

        result &= relations;
    }
    result &= check_indexing<CMap1::Vertex>(m);
    result &= check_indexing<CMap1::Edge>(m);
    result &= check_indexing<CMap1::Face>(m);
    result &= check_indexing<CMap1::Volume>(m);
    return result;
}

bool check_integrity(CMap2& m, bool verbose)
{
    bool result = true;
    for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
    {
        bool relations = true;
        relations &= phi2(m, d) != d && phi<2, 2>(m, d) == d;
        relations &= phi<-1, 1>(m, d) == d && phi<1, -1>(m, d) == d;
        if (verbose && !relations)
        {
            std::cerr << "Dart " << d << " has bad relations" << std::endl;
            if (phi2(m, d) == d)
                std::cerr << "  phi2 fixed point" << std::endl;
            if (phi<2, 2>(m, d) != d)
                std::cerr << "  phi2 not involution" << std::endl;
        }

        bool boundary =
            is_boundary(m, d) == is_boundary(m, phi1(m, d)) && (!is_boundary(m, d) || !is_boundary(m, phi2(m, d)));
        if (verbose && !boundary)
            std::cerr << "Dart " << d << " has bad boundary" << std::endl;

        result &= relations && boundary;
    }
    result &= check_indexing<CMap2::Vertex>(m);
    result &= check_indexing<CMap2::HalfEdge>(m);
    result &= check_indexing<CMap2::Edge>(m);
    result &= check_indexing<CMap2::Face>(m);
    result &= check_indexing<CMap2::Volume>(m);
    return result;
}


bool check_integrity(CMap3& m, bool verbose)
{
    bool result = true;
    for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
    {
        bool relations = true;
        relations &= phi3(m, d) != d && phi<3, 3>(m, d) == d && phi<3, 1, 3, 1>(m, d) == d;
        relations &= phi2(m, d) != d && phi<2, 2>(m, d) == d;
        relations &= phi<-1, 1>(m, d) == d && phi<1, -1>(m, d) == d;
        if (verbose && !relations)
            std::cerr << "Dart " << d << " has bad relations" << std::endl;

        bool boundary = is_boundary(m, d) == is_boundary(m, phi1(m, d)) &&
                        is_boundary(m, d) == is_boundary(m, phi2(m, d)) &&
                        (!is_boundary(m, d) || !is_boundary(m, phi3(m, d)));
        if (verbose && !boundary)
            std::cerr << "Dart " << d << " has bad boundary" << std::endl;

        result &= relations && boundary;
    }
    result &= check_indexing<CMap3::Vertex>(m);
    result &= check_indexing<CMap3::Vertex2>(m);
    result &= check_indexing<CMap3::HalfEdge>(m);
    result &= check_indexing<CMap3::Edge>(m);
    result &= check_indexing<CMap3::Edge2>(m);
    result &= check_indexing<CMap3::Face>(m);
    result &= check_indexing<CMap3::Face2>(m);
    result &= check_indexing<CMap3::Volume>(m);
    return result;
}



} // namespace cgogn
