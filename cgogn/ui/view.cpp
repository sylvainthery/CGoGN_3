/*******************************************************************************
 * CGoGN                                                                        *
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

#include <cgogn/ui/view.h>
#include <GLFW/glfw3.h>
#include <cgogn/rendering/gl_image.h>

using cgogn::rendering::GLVec2;
using cgogn::rendering::GLVec3d;
using cgogn::rendering::GLVec4d;
using cgogn::rendering::GLMat4d;

namespace cgogn
{

namespace ui
{

View::View(Inputs* inputs, const std::string& name)
	: GLViewer(inputs), name_(name), ratio_x_offset_(0), ratio_y_offset_(0), ratio_width_(1), ratio_height_(1),
	  param_final_simple_(nullptr), fbo_(nullptr), tex_(nullptr), event_stopped_(false), closing_(false),
	  nb_ao_blurs_(2), shift_zplane_(0.05f), hbao_radius_ratio_(0.01f), bias_k_div_(GLVec2(8.0f, 8.0f))
{
	tex_ = std::make_shared<cgogn::rendering::Texture2D>();
	tex_->allocate(1, 1, GL_RGBA8, GL_RGBA);
	tex_n_ = std::make_shared<cgogn::rendering::Texture2D>();
	tex_n_->allocate(1, 1, GL_RGB8, GL_RGB);

	fbo1_ = std::make_unique<cgogn::rendering::FBO>(std::vector<std::shared_ptr<cgogn::rendering::Texture2D>>{tex_}, true, nullptr);
	fbo2_ = std::make_unique<cgogn::rendering::FBO>(
		std::vector<std::shared_ptr<cgogn::rendering::Texture2D>>{tex_, tex_n_}, true, fbo1_.get());
	fbo_ = fbo1_.get();

	tex_hbao_ = std::make_shared<cgogn::rendering::Texture2D>();
	tex_hbao_->allocate(1, 1, GL_R16F, GL_RED);
	fbo_hbao_ = std::make_unique<cgogn::rendering::FBO>(std::vector<std::shared_ptr<cgogn::rendering::Texture2D>>{tex_hbao_},
												   false, nullptr);

	tex_hbao2_ = std::make_shared<cgogn::rendering::Texture2D>();
	tex_hbao2_->allocate(1, 1, GL_R16F, GL_RED);
	fbo_hbao2_ = std::make_unique<cgogn::rendering::FBO>(std::vector<std::shared_ptr<cgogn::rendering::Texture2D>>{tex_hbao2_},
												   false, nullptr);

	param_full_screen_hbao_ = cgogn::rendering::ShaderFullScreenHBAO::generate_param();
	param_full_screen_hbao_->tex_d_ = fbo2_->getDepthTexture();
	param_full_screen_hbao_->tex_n_ = fbo2_->getTexture(1);
	param_full_screen_hbao_->shadataptr_ = &shadow_;
	
	param_final_simple_ = cgogn::rendering::ShaderFullScreenTexture::generate_param();
	param_final_simple_->texture_ = tex_;

	param_full_screen_apply_ = cgogn::rendering::ShaderFullScreenApplyHBAO::generate_param();
	param_full_screen_apply_->tex_ambiant_ = fbo_hbao_->texture(0);
	param_full_screen_apply_->tex_diffuse_ = tex_;

	param_blur_ao_ = cgogn::rendering::ShaderFSBlurAO::generate_param();

	light_.link(camera_);
	sha_plane_.init(&shadow_);

}

View::~View()
{
}

void View::set_view_ratio(float64 px, float64 py, float64 pw, float64 ph)
{
	ratio_x_offset_ = px;
	ratio_y_offset_ = py;
	ratio_width_ = pw;
	ratio_height_ = ph;
}

void View::resize_event(int32 window_width, int32 window_height, int32 frame_buffer_width, int32 frame_buffer_height)
{
	x_offset_ = int32(ratio_x_offset_ * window_width);
	y_offset_ = int32(ratio_y_offset_ * window_height);
	width_ = int32(ratio_width_ * window_width);
	height_ = int32(ratio_height_ * window_height);

	viewport_x_offset_ = int32(ratio_x_offset_ * frame_buffer_width);
	viewport_y_offset_ = int32(ratio_y_offset_ * frame_buffer_height);

	GLViewer::resize_event(int32(ratio_width_ * frame_buffer_width), int32(ratio_height_ * frame_buffer_height));

	fbo1_->resize(viewport_width_, viewport_height_);
	fbo2_->resize(viewport_width_, viewport_height_);
	fbo_hbao_->resize(viewport_width_, viewport_height_);
	fbo_hbao2_->resize(viewport_width_, viewport_height_);
}

void View::close_event()
{
	for (ViewModule* m : linked_view_modules_)
		m->close_event();

	closing_ = true;
}

void View::mouse_press_event(int32 button, int32 x, int32 y)
{
	for (ViewModule* m : linked_view_modules_)
		m->mouse_press_event(this, button, x, y);

	if (!event_stopped_)
		GLViewer::mouse_press_event(button, x, y);
	event_stopped_ = false;
}

void View::mouse_release_event(int32 button, int32 x, int32 y)
{
	for (ViewModule* m : linked_view_modules_)
		m->mouse_release_event(this, button, x, y);

	if (!event_stopped_)
		GLViewer::mouse_release_event(button, x, y);
	event_stopped_ = false;
}

void View::mouse_dbl_click_event(int32 button, int32 x, int32 y)
{
	for (ViewModule* m : linked_view_modules_)
		m->mouse_dbl_click_event(this, button, x, y);

	if (!event_stopped_)
		GLViewer::mouse_dbl_click_event(button, x, y);
	event_stopped_ = false;
}

void View::mouse_move_event(int32 x, int32 y)
{
	for (ViewModule* m : linked_view_modules_)
		m->mouse_move_event(this, x, y);

	if (!event_stopped_)
		GLViewer::mouse_move_event(x, y);
	event_stopped_ = false;
}

void View::mouse_wheel_event(float64 dx, float64 dy)
{
	for (ViewModule* m : linked_view_modules_)
		m->mouse_wheel_event(this, int32(dx), int32(dy));

	if (!event_stopped_)
		GLViewer::mouse_wheel_event(dx, dy);
	event_stopped_ = false;
}

void View::key_press_event(int32 key_code)
{
	for (ViewModule* m : linked_view_modules_)
		m->key_press_event(this, key_code);

	if (!event_stopped_)
		GLViewer::key_press_event(key_code);
	event_stopped_ = false;
}

void View::key_release_event(int32 key_code)
{
	for (ViewModule* m : linked_view_modules_)
		m->key_release_event(this, key_code);

	if (!event_stopped_)
		GLViewer::key_release_event(key_code);
	event_stopped_ = false;
}


void View::draw()
{			  
	if (closing_)
		return;
	spin();

	float64 sr = (bb_.second - bb_.first).norm()/2.0;

	if (need_redraw_) //AND NEED SHADOW UPDATE
	{
		shadow_.bias_k_[0] = float32(sr) / std::pow(2.0f,bias_k_div_[0]);

		if (shadow_.is_started())
		{
			GLVec3d center = (bb_.first + bb_.second) / 2.0;
			GLVec3 wlpf = light_.getWorldCoord();
			GLVec3d wlp = wlpf.cast<double>();


			auto orthographic = [&](float64 zcenter) {
				float64 znear = zcenter - sr;
				float64 zfar = zcenter + sr;
				shadow_.bias_adapt_ = rendering::GLVec2{float32(1.0 / (znear * (zfar - znear))),
																		 float32((zfar - znear) / (zfar * znear))};
				float64 ihw = 1.0 / sr;
				float64 r_inv = 1.0 / (znear - zfar);
				GLMat4d m;
				m << ihw, 0, 0, 0, 0, ihw, 0, 0, 0, 0, 2.0 * r_inv, (znear + zfar) * r_inv, 0, 0, 0, 1;
				return m;
			};

			auto look_at = [](const GLVec3d& eye, const GLVec3d& at, const GLVec3d& up) {
				GLVec3d zAxis = (eye - at).normalized();
				GLVec3d xAxis = up.cross(zAxis).normalized();
				GLVec3d yAxis = zAxis.cross(xAxis).normalized();
				GLMat4d trf;
				trf.block<1, 3>(0, 0) = xAxis.transpose();
				trf.block<1, 3>(1, 0) = yAxis.transpose();
				trf.block<1, 3>(2, 0) = zAxis.transpose();
				trf.block<3, 1>(0, 3) = GLVec3d(-xAxis.dot(eye), -yAxis.dot(eye), -zAxis.dot(eye));
				trf.block<1, 4>(3, 0) = GLVec4d(0, 0, 0, 1).transpose();
				return trf;
			};

			GLMat4d biasmat;
			biasmat << 0.5, 0.0, 0.0, 0.5, 0.0, 0.5, 0.0, 0.5, 0.0, 0.0, 0.5, 0.5, 0.0, 0.0, 0.0, 1.0;

			GLMat4d light_projection_matrix = orthographic((wlp - center).norm());
			GLMat4d light_view_matrix = look_at(wlp, center, rendering::GLVec3d(0, 1, 0));
			shadow_.shadow_matrix_ =
				biasmat * light_projection_matrix * light_view_matrix * camera().modelview_matrix_d().inverse();

			shadow_.fbo_shadows_->bind();
			glEnable(GL_DEPTH_TEST);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
	
			for (ViewModule* m : linked_view_modules_)
				m->draw_shadowmap(this, light_projection_matrix, light_view_matrix);
			shadow_.fbo_shadows_->release();
			glCullFace(GL_BACK);
		}
	}

	glViewport(viewport_x_offset_, viewport_y_offset_, viewport_width_, viewport_height_);
	if (need_redraw_)
	{
		if (fbo_->width() * fbo_->height() > 0)
		{
			fbo_->bind();
			glEnable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (shadow_.is_started())
			{
				GLenum idbuf[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
				glDrawBuffers(2, idbuf);
			}
			else
			{
				GLenum idbuf = GL_COLOR_ATTACHMENT0;
				glDrawBuffers(1, &idbuf);
			}

			if (shadow_.is_started())
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				sha_plane_.drawZ(bb_, shift_zplane_, camera_.projection_matrix_d(), camera_.modelview_matrix_d(),
								 light_.getEyeCoord());
				glDisable(GL_BLEND);
			}

			for (ViewModule* m : linked_view_modules_)
				m->draw(this);

			for (ViewModule* m : linked_view_modules_)
				m->draw_no_shading(this);

			fbo_->release();

			if (shadow_.is_started())
			{
				fbo_hbao_->bind();
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);
				glDisable(GL_DEPTH_TEST);

				shadow_.fbo_shadows_->getDepthTexture()->bind();
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
				glGenerateMipmap(GL_TEXTURE_2D);
				shadow_.fbo_shadows_->getDepthTexture()->release();

				param_full_screen_hbao_->radius_ = float32(sr) * hbao_radius_ratio_;
				param_full_screen_hbao_->light_position_ = light_.getEyeCoord();

				GLVec3d Np = camera_.modelview_matrix_d().block<3, 1>(0, 2).normalized();
				GLVec3d Pw{(bb_.first.x() + bb_.second.x()) / 2, (bb_.first.y() + bb_.second.y()) / 2,
						   bb_.first.z() - shift_zplane_ * (bb_.second.z() - bb_.first.z())};
				GLVec3d Pp = cgogn::rendering::homoTransform(camera_.modelview_matrix_d(), Pw);

				param_full_screen_hbao_->plane_ = GLVec4d{Np.x(), Np.y(), Np.z(), Np.dot(Pp)}.cast<float>();

				param_full_screen_hbao_->draw(this);
				fbo_hbao_->release();

//				shadow_.fbo_shadows_->getDepthTexture()->bind();
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
//				shadow_.fbo_shadows_->getDepthTexture()->release();

				for (int i = 0; i < nb_ao_blurs_; ++i)
				{
					fbo_hbao2_->bind();
					param_blur_ao_->tex_ = fbo_hbao_->getTexture(0);
					param_blur_ao_->blurH();
					fbo_hbao2_->release();

					fbo_hbao_->bind();
					param_blur_ao_->tex_ = fbo_hbao2_->getTexture(0);
					param_blur_ao_->blurV();
					fbo_hbao_->release();
				}
			}
			need_redraw_ = false;
		}
	}
 
	if (shadow_.is_started())
		param_full_screen_apply_->draw();
	else
		param_final_simple_->draw();
}


void View::link_module(ViewModule* m)
{
	if (std::find(linked_view_modules_.begin(), linked_view_modules_.end(), m) == linked_view_modules_.end())
	{
		linked_view_modules_.push_back(m);
		m->linked_views_.push_back(this);
	}
}

void View::link_module(ProviderModule* m)
{
	if (std::find(linked_provider_modules_.begin(), linked_provider_modules_.end(), m) ==
		linked_provider_modules_.end())
	{
		linked_provider_modules_.push_back(m);
		m->linked_views_.push_back(this);
	}
}

void View::update_scene_bb()
{
	GLVec3d& min = bb_.first; 
	GLVec3d& max = bb_.second; 
	for (uint32 i = 0; i < 3; ++i)
	{
		min[i] = std::numeric_limits<float64>::max();
		max[i] = std::numeric_limits<float64>::lowest();
	}
	for (ProviderModule* m : linked_provider_modules_)
	{
		auto [pmin, pmax] = m->meshes_bb();
		for (uint32 i = 0; i < 3; ++i)
		{
			if (pmin[i] < min[i])
				min[i] = pmin[i];
			if (pmax[i] > max[i])
				max[i] = pmax[i];
		}
	}
	geometry::Scalar radius = (max - min).norm() / 2.0;
	geometry::Vec3 center = (max + min) / 2.0;
	std::cout << "UPDATESCENE " << center.transpose() << "  " << radius << std::endl;
	set_scene_radius(radius);
	set_scene_center(center);
	show_entire_scene();
	request_update();
}

bool View::pixel_scene_position(int32 x, int32 y, GLVec3d& P) const
{
	float z[4];
	GLint xs, ys;
	float64 xogl;
	float64 yogl;
	float64 zogl;

	xs = GLint(double(x - x_offset_) / double(width_) * viewport_width_);
	ys = GLint(double(height_ - (y - y_offset_)) / double(height_) * viewport_height_);

	fbo_->bind_read();
	glReadBuffer(GL_DEPTH_ATTACHMENT);
	glReadPixels(xs, ys, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, z);
	fbo_->release_read();

	if (*z >= 1.0f)
		return false;

	xogl = (float64(xs) / viewport_width_) * 2.0 - 1.0;
	yogl = (float64(ys) / viewport_height_) * 2.0 - 1.0;
	zogl = float64(*z) * 2.0 - 1.0;

	GLVec4d Q(xogl, yogl, zogl, 1.0);
	rendering::GLMat4d im = (camera().projection_matrix_d() * camera().modelview_matrix_d()).inverse();
	GLVec4d P4 = im * Q;
	if (P4.w() != 0.0)
	{
		P.x() = P4.x() / P4.w();
		P.y() = P4.y() / P4.w();
		P.z() = P4.z() / P4.w();
		return true;
	}

	return false;
}

std::pair<GLVec3d, GLVec3d> View::pixel_ray(int32 x, int32 y) const
{
	float64 xs = float64(float64(x - x_offset_) / float64(width_) * viewport_width_);
	float64 ys = float64(float64(height_ - (y - y_offset_)) / float64(height_) * viewport_height_);

	float64 xogl = (xs / viewport_width_) * 2.0 - 1.0;
	float64 yogl = (ys / viewport_height_) * 2.0 - 1.0;

	rendering::GLMat4d im = (camera().projection_matrix_d() * camera().modelview_matrix_d()).inverse();
	GLVec4d Q(xogl, yogl, 1.0, 1.0);
	GLVec4d P4 = im * Q;

	GLVec3d P1(P4.x() / P4.w(), P4.y() / P4.w(), P4.z() / P4.w());

	Q.z() = -1;
	P4 = im * Q;
	GLVec3d P2(P4.x() / P4.w(), P4.y() / P4.w(), P4.z() / P4.w());
	return std::make_pair(P1, P2);
}

GLVec3d View::unproject(int32 x, int32 y, float64 z) const
{
	float64 xogl = (double(x - x_offset_) / double(width_)) * 2.0 - 1.0;
	float64 yogl = (double(height_ - (y - y_offset_)) / double(height_)) * 2.0 - 1.0;
	float64 zogl = z * 2.0 - 1.0;

	GLVec4d Q(xogl, yogl, zogl, 1.0);
	rendering::GLMat4d im = (camera().projection_matrix_d() * camera().modelview_matrix_d()).inverse();
	GLVec4d res = im * Q;
	res /= res.w();
	return res.head(3);
}

void View::save_screenshot()
{
	std::string filename = "screenshot.jpg";
	std::cout << "saving screenshot : " << filename << std::endl;

	if (fbo_->width() * fbo_->height() > 0)
	{
		rendering::GLImage image(viewport_width_, viewport_height_, 3);
		const int nb_pixels = viewport_width_ * viewport_height_;

		fbo_->bind();
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadBuffer(GL_DRAW_FRAMEBUFFER);
		glReadPixels(0, 0, viewport_width_, viewport_height_, GL_RGB, GL_UNSIGNED_BYTE,
					 const_cast<uint8*>(image.data()));
		fbo_->release();

		image.save(filename, true);
	}
}

} // namespace ui

} // namespace cgogn
                               
