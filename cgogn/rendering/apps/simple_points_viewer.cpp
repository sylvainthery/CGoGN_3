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

#include <cgogn/geometry/types/vector_traits.h>

#include <cgogn/ui/app.h>
#include <cgogn/ui/view.h>

#include <cgogn/core/ui_modules/mesh_provider.h>
#include <cgogn/rendering/ui_modules/volume_render.h>

#include <cgogn/rendering/ui_modules/points_cloud_render.h>


#include <cgogn/geometry/algos/centroid.h>
#include <cgogn/geometry/functions/distance.h>

#include <cgogn/modeling/algos/volume_utils.h>

#include <cgogn/core/utils/numerics.h>
#include <cgogn/io/utils.h>
#include <cgogn/io/volume/volume_import.h>

#include <vector>
#include <random>
#include <chrono>

#include <cgogn/core/functions/cells.h>

#define DEFAULT_MESH_PATH CGOGN_STR(CGOGN_DATA_PATH) "/meshes/"

using Vec3 = cgogn::geometry::Vec3;
using Scalar = cgogn::geometry::Scalar;

using PointCloud =cgogn::CMap0;
template <typename T>
using PCAttribute = typename cgogn::mesh_traits<PointCloud>::Attribute<T>;
using namespace cgogn::numerics;

using PC_Vertex = typename cgogn::mesh_traits<PointCloud>::Vertex;


//#define PERF_TEST


int main(int argc, char** argv)
{

	cgogn::thread_start();

	cgogn::ui::App app;
	app.set_window_title("Delaunay volume viewer");
	app.set_window_size(1000, 800);

	cgogn::ui::MeshProvider<PointCloud> mpc(app);

	PointCloud pc;
	std::shared_ptr<PCAttribute<Vec3>> pc_vertex_position = cgogn::add_attribute<Vec3, PC_Vertex>(pc, "position");
	/*cgogn::init_cells_indexing<PC_Vertex>(pc);*/
	cgogn::Dart d1 = add_dart(pc);
	cgogn::value<Vec3>(pc, pc_vertex_position, PC_Vertex(d1)) = Vec3(1.5, 1.5, 1.5);
	cgogn::Dart d2 = add_dart(pc);
	cgogn::value<Vec3>(pc, pc_vertex_position, PC_Vertex(d2)) = Vec3(1.25, 1.25, 1.25);

	cgogn::ui::VolumeRender<Mesh> vr(app);

	cgogn::ui::PointsCloudRender<PointCloud> pcr(app);

	app.init_modules();

	cgogn::ui::View* v1 = app.current_view();
	v1->link_module(&mpc);
	v1->link_module(&pcr);

	mpc.set_mesh_bb_vertex_position(*m, vertex_position);
	mpc.set_vertex_position(*v1, *m, vertex_position);

	return app.launch();
}
