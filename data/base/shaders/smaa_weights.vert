// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// SMAA 1x blending weight calculation pass, vertex shader port.
// Based on SMAA (Enhanced Subpixel Morphological Antialiasing), http://www.iryoku.com/smaa/
//
// Copyright (C) 2013 Jorge Jimenez (jorge@iryoku.com)
// Copyright (C) 2013 Jose I. Echevarria (joseignacioechevarria@gmail.com)
// Copyright (C) 2013 Belen Masia (bmasia@unizar.es)
// Copyright (C) 2013 Fernando Navarro (fernandn@microsoft.com)
// Copyright (C) 2013 Diego Gutierrez (diegog@unizar.es)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to
// do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software. As clarification, there
// is no requirement that the copyright notice and permission be included in
// binary distributions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// xy = 1 / input size, zw = input size (in physical texels)
layout(std140) uniform cbuffer {
	vec4 rtMetrics;
	vec4 uvScaleClamp;
	vec4 params;
};
// xy scales viewport spanning texcoords down to the rendered sub-rect of the
// input, zw clamps taps just inside its edge (both are identity-like at full size)
// x = max orthogonal search steps, y = max diagonal search steps (0 disables
// diagonal processing), z = corner rounding [0..1] (1 disables corner processing)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#ifdef NEWGL
in vec2 vertexPos;
out vec2 uv;
out vec2 pixcoord;
out vec4 offset0;
out vec4 offset1;
out vec4 offset2;
#else
attribute vec2 vertexPos;
varying vec2 uv;
varying vec2 pixcoord;
varying vec4 offset0;
varying vec4 offset1;
varying vec4 offset2;
#endif

void main()
{
	gl_Position = vec4(vertexPos.x, vertexPos.y, 0.0, 1.0);
	uv = (0.5 * gl_Position.xy + vec2(0.5)) * uvScaleClamp.xy;
	pixcoord = uv * rtMetrics.zw;

	// offsets for the pseudo gather4 crossing edge fetches
	offset0 = uv.xyxy + rtMetrics.xyxy * vec4(-0.25, -0.125,  1.25, -0.125);
	offset1 = uv.xyxy + rtMetrics.xyxy * vec4(-0.125, -0.25, -0.125,  1.25);

	// the far ends of the orthogonal search loops
	offset2 = vec4(offset0.xz, offset1.yw) + vec4(-2.0, 2.0, -2.0, 2.0) * rtMetrics.xxyy * params.x;
}
