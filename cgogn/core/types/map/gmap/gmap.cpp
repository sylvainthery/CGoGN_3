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
#include <iomanip>
#include <cgogn/core/types/map/gmap/gmap3.h>
#include <cgogn/core/functions/mesh_info.h>

namespace cgogn
{

void dump_map_darts(const GMapBase& m)
{
	for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
	{
		std::cout << "index: " << std::setw(5) << d.index << " / ";
		for (auto& r : m.relations_)
			std::cout << r->name() << ": " << std::setw(5) << (*r)[d.index] << " / ";
//		for (auto& ind : m.cells_indices_)
//			if (ind)
//				std::cout << ind->name() << ": " << std::setw(5) << (*ind)[d.index] << " / ";
		for (uint32 orb : m.cells_embedded_orbit_)
		{
			auto& ind = m.cells_indices_[orb];
			std::cout << ind->name() << ": " << std::setw(5) << (*ind)[d.index] << " / ";
		}
	}
}


bool check_integrity(GMap1& m, bool verbose)
{
	bool result = true;
	for (Dart d = m.begin(), end = m.end(); d != end; d = m.next(d))
	{
		//		bool relations = phi<-1, 1>(m, d) == d && phi<1, -1>(m, d) == d;
		//		if (verbose && !relations)
		//			std::cerr << "Dart " << d << " has bad relations" << std::endl;

		//		result &= relations;
	}
	result &= check_indexing<GMap1::Vertex>(m);
	result &= check_indexing<GMap1::Edge>(m);
	result &= check_indexing<GMap1::Face>(m);
	return result;
}


GMap0::Edge add_edge(GMap0& m, bool set_indices)
{
	Dart d = add_dart(m);
	Dart e = add_dart(m);
	beta0_sew(m, d, e);

	GMap0::Edge edge{d};

	if (set_indices)
	{
		if (is_indexed<GMap0::Vertex>(m))
		{
			set_index(m, GMap0::Vertex{d}, new_index<GMap0::Vertex>(m));
			set_index(m, GMap0::Vertex{e}, new_index<GMap0::Vertex>(m));
		}

		if (is_indexed<GMap0::Edge>(m))
		{
			set_index(m, edge, new_index<GMap0::Edge>(m));
		}
	}
	return edge;
}

void remove_edge(GMap0& m, GMap0::Edge e, bool set_indice)
{
	remove_dart(m, e.dart);
	remove_dart(m, beta0(m, e.dart));
}

GMap1::Face add_face(GMap1& m, uint32 size, bool set_indices)
{
	using Vertex = GMap1::Vertex;
	using Edge = GMap1::Edge;
	using Face = GMap1::Face;

	Edge e0 = add_edge(m, false);
	Edge ep = e0;
	for (int i = 1; i < size; ++i)
	{
		Edge e = add_edge(m, false);
		beta1_sew(m, e.dart, beta0(m, ep.dart));
		ep = e;
	}
	beta1_sew(m, e0.dart, beta0(m, ep.dart));


	if (set_indices)
	{
		for (int i = 0; i < size; ++i)
		{
			Edge e{phi1(m, ep.dart)};
			if (is_indexed<Vertex>(m))
				set_index(m, Vertex{e.dart}, new_index<Vertex>(m));
			if (is_indexed<Edge>(m))
				set_index(m, e, new_index<Edge>(m));

			ep = e;
		}
		if (is_indexed<Face>(m))
			set_index(m, Face{ep.dart}, new_index<Face>(m));
	}
	return Face{e0.dart};
}

void remove_face(GMap1& m, GMap1::Face f)
{
	Dart it = phi1(m, f.dart);
	while (it != f.dart)
	{
		Dart next = phi1(m, it);
		remove_edge(m, GMap1::Edge{it});
		it = next;
	}
	remove_dart(m, f.dart);
}


GMap1::Vertex cut_edge(GMap1& m, GMap1::Edge e, bool set_indices)
{
	using Vertex = GMap1::Vertex;
	using Edge = GMap1::Edge;
	using Face = GMap1::Face;

	Dart e0 = e.dart;
	Dart e1 = beta0(m, e0);

	Dart vd0 = add_dart(m);
	Dart vd1 = add_dart(m);

	beta0_sew(m, e0, vd0);
	beta0_sew(m, e1, vd1);
	beta1_sew(m, vd0, vd1);

	Vertex v(vd0);

	if (set_indices)
	{
		if (is_indexed<Vertex>(m))
			set_index(m, v, new_index<Vertex>(m));
		if (is_indexed<Edge>(m))
		{
			copy_index<Edge>(m, vd0, e.dart);
			copy_index<Edge>(m, vd1, e.dart);
		}

		if (is_indexed<Face>(m))
		{
			copy_index<Face>(m, vd0, e.dart);
			copy_index<Face>(m, vd1, e.dart);
		}
	}

	return v;
}


GMap1::Vertex collapse_edge(GMap1& m, GMap1::Edge e, bool set_indices)
{
	using Vertex = GMap1::Vertex;
	Dart d1 = beta1(m, e.dart);
	Dart d2 = beta1(m, beta0(m, e.dart));

	beta1_unsew(m, d1);
	beta1_unsew(m, d2);

	beta1_sew(m, d1, d2);

	remove_edge(m, e);

	Vertex v(d1);

	if (set_indices)
		copy_index<Vertex>(m, d2, d1);

	return v;
}



GMap2::Vertex cut_edge(GMap2& m, GMap2::Edge e, bool set_indices)
{
	Dart e2 = beta2(m, e.dart);
	Dart d1 = (cut_edge(static_cast<GMap1&>(m), GMap1::Edge{e.dart}, false)).dart;
	Dart d2 = (cut_edge(static_cast<GMap1&>(m), GMap1::Edge{e2}, false)).dart;
	beta2_sew(m, d1, d2);
	beta2_sew(m, beta1(m, d1), beta1(m, d2));

	GMap2::Vertex vert{d1};
	if (set_indices)
	{
		if (is_indexed<GMap2::Vertex>(m))
			set_index(m, vert, new_index<GMap2::Vertex>(m));

		if (is_indexed<GMap2::Edge>(m))
		{
			uint32 ind = index_of(m, GMap2::Edge(e.dart));
			set_index<GMap2::Edge>(m, d1, ind);
			set_index<GMap2::Edge>(m, d2, ind);
			set_index<GMap2::Edge>(m, beta1(m, d1), ind);
			set_index<GMap2::Edge>(m, beta1(m, d2), ind);
		}

		if (is_indexed<GMap2::Face>(m))
		{
			uint32 ind1 = index_of(m, GMap2::Face(e.dart));
			set_index<GMap2::Edge>(m, d1, ind1);
			set_index<GMap2::Edge>(m, beta1(m, d1), ind1);
			uint32 ind2 = index_of(m, GMap2::Face(e2));
			set_index<GMap2::Edge>(m, d2, ind2);
			set_index<GMap2::Edge>(m, beta1(m, d2), ind2);
		}
	}
	return vert;
}

// WARNING v1.dart & v2.dart must belong to the same  orientation
GMap2::Edge cut_face(GMap2& m, GMap2::Vertex v1, GMap2::Vertex v2, bool set_indices)
{
	using Vertex = GMap2::Vertex;
	using Edge = GMap2::Edge;
	using Face = GMap2::Face;

	Dart d1 = v1.dart;
	Dart dd1 = beta1(m, d1);
	Dart d2 = v2.dart;
	Dart dd2 = beta1(m, d2);

	beta1_unsew(m, d1);
	beta1_unsew(m, d2);

	Dart e1 = add_edge(m, false).dart;
	Dart e2 = add_edge(m, false).dart;

	beta1_sew(m, d1, e1);
	beta1_sew(m, d2, e2);
	beta1_sew(m, dd1, beta0(m, e2));
	beta1_sew(m, dd2, beta0(m, e1));

	beta2_sew(m, e1, beta0(m, e2));
	beta2_sew(m, e2, beta0(m, e1));

	if (set_indices)
	{
		if (is_indexed<Vertex>(m))
		{
			uint32 ind1 = index_of(m, v1);
			set_index<Vertex>(m, e1, ind1);
			set_index<Vertex>(m, beta0(m, e2), ind1);
			uint32 ind2 = index_of(m, v2);
			set_index<Vertex>(m, e2, ind2);
			set_index<Vertex>(m, beta0(m, e1), ind2);
		}
		if (is_indexed<Edge>(m))
			set_index(m, Edge{e1}, new_index<Edge>(m));

		if (is_indexed<Face>(m))
		{
			uint32 ind = index_of(m, Face(v1.dart));
			set_index<Face>(m, e1, ind);
			set_index<Face>(m, beta0(m, e2), ind);
			set_index<Face>(m, e2, ind);
			set_index<Face>(m, beta0(m, e1), ind);
		}
	}
	return Edge{e1};
}




} // namespace cgogn
