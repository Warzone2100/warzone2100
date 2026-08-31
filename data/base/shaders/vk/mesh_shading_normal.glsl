#ifndef WZ_MESH_SHADING_NORMAL_GLSL
#define WZ_MESH_SHADING_NORMAL_GLSL

// Geometric vertex shading normal in world space.
// Classic / object-space meshes store inverted vertex N (hasTangents == 0);
// tangent-space meshes store outward N. Lighting and the scene-prepass G-buffer
// must use the same path so SSAO (and later SSR) see outward view-space N.
// View-space N is CPU ViewMatrix space -- do not Y-flip it for Vulkan clip.
vec3 wzWorldShadingNormal(mat3 normalMatrix, vec3 vertexNormal, int hasTangents)
{
	vec3 n = normalize(normalMatrix * vertexNormal);
	return (hasTangents != 0) ? n : -n;
}

vec3 wzViewShadingNormal(mat3 viewMatrix, mat3 normalMatrix, vec3 vertexNormal, int hasTangents)
{
	return viewMatrix * wzWorldShadingNormal(normalMatrix, vertexNormal, hasTangents);
}

#endif
