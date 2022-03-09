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

#include <cgogn/core/functions/mesh_info.h>
#include <cgogn/core/functions/attributes.h>

namespace cgogn
{

void dump_map_darts(const CMapBase& m)
{
	for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
	{
		std::cout << "index: " << std::setw(5) << d.index << " / ";
		for (auto& r : m.relations_)
			std::cout << r->name() << ": " << std::setw(5) << (*r)[d.index] << " / ";
		for (auto& ind : m.cells_indices_)
			if (ind)
				std::cout << ind->name() << ": " << std::setw(5) << (*ind)[d.index] << " / ";
		std::cout << " boundary: " << std::boolalpha << is_boundary(m, d) << std::endl;
	}
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
	return result;
}



bool edge_can_collapse(const CMap2& m, CMap2::Edge e)
{
	using Vertex = CMap2::Vertex;
	using Face = CMap2::Face;

	auto vertices = incident_vertices(m, e);

	if (is_incident_to_boundary(m, vertices[0]) || is_incident_to_boundary(m, vertices[1]))
		return false;

	uint32 val_v1 = degree(m, vertices[0]);
	uint32 val_v2 = degree(m, vertices[1]);

	if (val_v1 + val_v2 < 8 || val_v1 + val_v2 > 14)
		return false;

	Dart e1 = e.dart;
	Dart e2 = phi2(m, e.dart);
	if (codegree(m, Face(e1)) == 3)
	{
		if (degree(m, Vertex(phi_1(m, e1))) < 4)
			return false;
	}
	if (codegree(m, Face(e2)) == 3)
	{
		if (degree(m, Vertex(phi_1(m, e2))) < 4)
			return false;
	}

	auto next_edge = [&m](Dart d) { return phi<-1, 2>(m, d); };

	// Check vertex sharing condition
	std::vector<uint32> vn1;
	Dart it = next_edge(next_edge(e1));
	Dart end = phi1(m, e2);
	do
	{
		vn1.push_back(index_of(m, Vertex(phi1(m, it))));
		it = next_edge(it);
	} while (it != end);
	it = next_edge(next_edge(e2));
	end = phi1(m, e1);
	do
	{
		auto vn1it = std::find(vn1.begin(), vn1.end(), index_of(m, Vertex(phi1(m, it))));
		if (vn1it != vn1.end())
			return false;
		it = next_edge(it);
	} while (it != end);

	return true;
}

bool edge_can_flip(const CMap2& m, CMap2::Edge e)
{
	if (is_incident_to_boundary(m, e))
		return false;

	Dart e1 = e.dart;
	Dart e2 = phi2(m, e1);

	auto next_edge = [&m](Dart d) { return phi<-1, 2>(m, d); };

	if (codegree(m, CMap2::Face(e1)) == 3 && codegree(m, CMap2::Face(e2)) == 3)
	{
		uint32 idxv2 = index_of(m, CMap2::Vertex(phi_1(m, e2)));
		Dart d = phi_1(m, e1);
		Dart it = d;
		do
		{
			if (index_of(m, CMap2::Vertex(phi1(m, it))) == idxv2)
				return false;
			it = next_edge(it);
		} while (it != d);
	}

	return true;
}

} // namespace cgogn

