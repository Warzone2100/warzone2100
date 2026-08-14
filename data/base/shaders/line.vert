// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#include "rect_common.glsl"

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
#else
attribute vec4 vertex;
#endif


void main()
{
	// this program uses the two vec2 slots of the shared layout as the segment endpoints
	vec4 pos = vec4(tuv_offset + (tuv_scale - tuv_offset)*vertex.y, 0.0, 1.0);
	gl_Position = transformationMatrix * pos;
}
