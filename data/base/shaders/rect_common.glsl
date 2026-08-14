// One constant layout, shared by the rect, textured rect, text, generic colour, line and
// gfx text programs, whose C++ structs are deliberately laid out the same way. A named
// block must be declared identically in every stage, and rect.frag pairs with three
// different vertex shaders, so they all share this declaration.
//
// The two vec2 slots carry UV offset and scale for the rect and text programs, and the
// segment endpoints for the line program.

layout(std140) uniform cbuffer {
	mat4 transformationMatrix;
	vec2 tuv_offset;
	vec2 tuv_scale;
	vec4 color;
};
