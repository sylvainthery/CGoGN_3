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
 * for more details.            

 */

#ifndef CGOGN_RENDERING_SHADERS_EXPLODE_VOLUMES_DEF_H_
#define CGOGN_RENDERING_SHADERS_EXPLODE_VOLUMES_DEF_H_

#include <cgogn/rendering/cgogn_rendering_export.h>
#include <cgogn/rendering/shader_program.h>
#include <cgogn/rendering/shadows.h>
#include <cgogn/rendering/shaders/shader_explode_volumes_data.h>
#include <cgogn/rendering/fbo.h>

namespace cgogn
{

namespace rendering
{

DECLARE_SHADER_CLASS(ExplodeVolumes1, true, CGOGN_STR(ExplodeVolumes1))

class CGOGN_RENDERING_EXPORT ShaderParamExplodeVolumes1 : public ShaderParam
{
	void set_uniforms() override;

	std::array<VBO*, 3> vbos_;
	inline void set_texture_buffer_vbo(uint32 i, VBO* vbo) override
	{
		vbos_[i] = vbo;
	}
	void bind_texture_buffers() override;
	void release_texture_buffers() override;

	enum VBOName : int32
	{
		VERTEX_POSITION = 0,
		VOLUME_CENTER,
		VOLUME_CLIPPING
	};

public:
	ExplodeVolumeData* data_;

	using ShaderType = ShaderExplodeVolumes1;

	inline ShaderParamExplodeVolumes1(ShaderType* sh) : ShaderParam(sh), data_(nullptr)
	{
		for (auto& v : vbos_)
			v = nullptr;
	}

	inline ~ShaderParamExplodeVolumes1() override
	{
	}
};



DECLARE_SHADER_CLASS(ExplodeVolumes1smooth, true, CGOGN_STR(ExplodeVolumes1smooth))

class CGOGN_RENDERING_EXPORT ShaderParamExplodeVolumes1smooth : public ShaderParam
{
	void set_uniforms() override;

	std::array<VBO*, 3> vbos_;
	inline void set_texture_buffer_vbo(uint32 i, VBO* vbo) override
	{
		vbos_[i] = vbo;
	}
	void bind_texture_buffers() override;
	void release_texture_buffers() override;

	enum VBOName : int32
	{
		VERTEX_POSITION = 0,
		VOLUME_CENTER,
		VOLUME_CLIPPING
	};

public:
	ExplodeVolumeData* data_;

	using ShaderType = ShaderExplodeVolumes1smooth;

	inline ShaderParamExplodeVolumes1smooth(ShaderType* sh) : ShaderParam(sh), data_(nullptr)
	{
		for (auto& v : vbos_)
			v = nullptr;
	}

	inline ~ShaderParamExplodeVolumes1smooth() override
	{
	}
};



DECLARE_SHADER_CLASS(ExplodeVolumes2, true, CGOGN_STR(ExplodeVolumes2))

class CGOGN_RENDERING_EXPORT ShaderParamExplodeVolumes2 : public ShaderParam
{
	void set_uniforms() override;

public:
	std::shared_ptr<Texture2D> tex_n_;
	std::shared_ptr<Texture2D> tex_d_;
	GLVec3 fn_;
	GLVec2 projv_;
	ExplodeVolumeData* data_;
	ShadowData* sha_data_;

	using ShaderType = ShaderExplodeVolumes2;

	inline ShaderParamExplodeVolumes2(ShaderType* sh)
		: ShaderParam(sh), tex_n_(nullptr), tex_d_(nullptr), sha_data_(nullptr)
	{
	}

	inline ~ShaderParamExplodeVolumes2() override
	{
	}
};




} // namespace rendering

} // namespace cgogn

#endif
