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


	using ShaderType = ShaderExplodeVolumes2;

	inline ShaderParamExplodeVolumes2(ShaderType* sh) : ShaderParam(sh), sha_data_(nullptr)
	{}

	inline ~ShaderParamExplodeVolumes2() override
	{}

	inline void draw()
	{
		bind();
		glDrawArrays(GL_TRIANGLES, 0, 3);
		release();
	}

};

} // namespace rendering

} // namespace cgogn

#endif
