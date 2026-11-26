// Every stage that writes the final clip-space position (the vertex shader of a program without tessellation,
// or a tessellation evaluation shader) computes it in Vulkan clip space and ends main() with:
//
//     gl_Position = wzFinalizeClip(gl_Position);
//
// Vertex shaders that only feed tessellation control points are marked WZ_CLIP_NOT_FINAL instead.
// (Checked by cmake/WZCheckVulkanShaderConventions.cmake.)
//
// Set WZ_CLIP_SPACE_Y_UP to 1 for targets whose clip space has +y up.

#ifndef WZ_CLIP_SPACE_Y_UP
#define WZ_CLIP_SPACE_Y_UP 0
#endif

vec4 wzFinalizeClip(vec4 vkClipPos)
{
#if WZ_CLIP_SPACE_Y_UP
	return vec4(vkClipPos.x, -vkClipPos.y, vkClipPos.z, vkClipPos.w);
#else
	return vkClipPos;
#endif
}
