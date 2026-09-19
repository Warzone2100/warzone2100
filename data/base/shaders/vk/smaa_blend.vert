#version 450

// SMAA 1x neighborhood blending pass, vertex shader port.
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

// rtMetrics: xy = 1 / input size, zw = input size (in physical texels)
// uvScaleClamp: xy scales viewport spanning texcoords down to the rendered
// sub-rect of the input, zw clamps taps just inside its edge
layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 rtMetrics;
	vec4 uvScaleClamp;
};

layout(location = 0) in vec2 vertexPos;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 offset0;

void main()
{
	gl_Position = vec4(vertexPos.x, vertexPos.y, 0.0, 1.0);
	uv = (0.5 * gl_Position.xy + vec2(0.5)) * uvScaleClamp.xy;
	offset0 = uv.xyxy + rtMetrics.xyxy * vec4(1.0, 0.0, 0.0, 1.0);
}
