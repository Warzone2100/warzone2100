///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2013 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-07-07
// Updated : 2010-07-07
// Licence : This source is under MIT License
// File    : glm/gtx/int_10_10_10_2.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm
{
	GLM_FUNC_QUALIFIER dword uint10_10_10_2_cast
	(
		glm::vec4 const & v
	)
	{
		return dword(uint(v.x * 2047.f) << 0 | uint(v.y * 2047.f) << 10 | uint(v.z * 2047.f) << 20 | uint(v.w * 3.f) << 30);
	}
}//namespace glm
