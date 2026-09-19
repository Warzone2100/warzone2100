#version 450

// SMAA 1x luma edge detection pass, fragment shader port.
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
// params: x = luma contrast threshold
layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 rtMetrics;
	vec4 uvScaleClamp;
	vec4 params;
};

layout(set = 1, binding = 0) uniform sampler2D colorTex;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 offset0;
layout(location = 2) in vec4 offset1;
layout(location = 3) in vec4 offset2;

layout(location = 0) out vec4 FragColor;

vec3 sampleColor(vec2 p)
{
	return texture(colorTex, min(p, uvScaleClamp.zw)).rgb;
}

void main()
{
	// Rec. 709 luma
	vec3 lumaWeights = vec3(0.2126, 0.7152, 0.0722);

	float L     = dot(sampleColor(uv), lumaWeights);
	float Lleft = dot(sampleColor(offset0.xy), lumaWeights);
	float Ltop  = dot(sampleColor(offset0.zw), lumaWeights);

	vec4 delta;
	delta.xy = abs(L - vec2(Lleft, Ltop));
	vec2 edges = step(vec2(params.x), delta.xy);

	if (dot(edges, vec2(1.0)) == 0.0)
	{
		FragColor = vec4(0.0);
		return;
	}

	float Lright  = dot(sampleColor(offset1.xy), lumaWeights);
	float Lbottom = dot(sampleColor(offset1.zw), lumaWeights);
	delta.zw = abs(L - vec2(Lright, Lbottom));

	vec2 maxDelta = max(delta.xy, delta.zw);

	float Lleftleft = dot(sampleColor(offset2.xy), lumaWeights);
	float Ltoptop   = dot(sampleColor(offset2.zw), lumaWeights);
	delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop));

	maxDelta = max(maxDelta, delta.zw);
	float finalDelta = max(maxDelta.x, maxDelta.y);

	// local contrast adaptation, drop edges neighboring much stronger ones
	edges *= step(finalDelta, 2.0 * delta.xy);

	FragColor = vec4(edges, 0.0, 0.0);
}
