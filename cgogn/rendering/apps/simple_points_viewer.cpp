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

using PointsCloud =cgogn::CMap0;
template <typename T>
using PCAttribute = typename cgogn::mesh_traits<PointsCloud>::Attribute<T>;
using namespace cgogn::numerics;

using PC_Vertex = typename cgogn::mesh_traits<PointsCloud>::Vertex;



void create_pointscloud(PointsCloud* pc, int32 nb)
{
	std::random_device rd;	// Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<double> dis(-1.0, 1.0);

	cgogn::init_cells_indexing<PC_Vertex>(*pc);
	std::shared_ptr<PCAttribute<Vec3>> pc_vertex_position = cgogn::get_attribute<Vec3, PC_Vertex>(*pc, "position");

	std::shared_ptr<PCAttribute<float64>> pc_vertex_radius = cgogn::add_attribute<float64, PC_Vertex>(*pc, "radius");


	for (int32 i = 0; i < nb; ++i)
	{
		PC_Vertex v(cgogn::add_dart(*pc));
		cgogn::set_index(*pc, v, cgogn::new_index<PC_Vertex>(*pc));
		cgogn::value<Vec3>(*pc, pc_vertex_position, v ) = Vec3(dis(gen), dis(gen), dis(gen));
		cgogn::value<float64>(*pc, pc_vertex_radius, v) = 0.05+0.01*dis(gen);
	}
}


int main(int argc, char** argv)
{

	cgogn::thread_start();

	cgogn::ui::App app;
	app.set_window_title("pointq cloud viewer");
	app.set_window_size(1000, 800);

	cgogn::ui::MeshProvider<PointsCloud> mpc(app);

	PointsCloud *pc = new PointsCloud;
	mpc.register_mesh(pc,"points cloud");
	std::shared_ptr<PCAttribute<Vec3>> pc_vertex_position = cgogn::add_attribute<Vec3, PC_Vertex>(*pc, "position");

	create_pointscloud(pc, 1000);

	cgogn::ui::PointsCloudRender<PointsCloud> pcr(app);

	app.init_modules();

	cgogn::ui::View* v1 = app.current_view();
	v1->link_module(&mpc);
	v1->link_module(&pcr);

	mpc.set_mesh_bb_vertex_position(*pc, pc_vertex_position);
	pcr.set_vertex_position(*v1, *pc, pc_vertex_position);

	return app.launch();
}
