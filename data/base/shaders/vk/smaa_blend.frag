#version 450

// SMAA 1x neighborhood blending pass, fragment shader port.
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
// Neighborhood blending mixes in approximate linear light instead of the
// stored gamma encoded values, keeping blends across high contrast edges
// from losing perceived brightness. Comment out for reference behavior.
#define WZ_SMAA_GAMMA2_BLEND

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 rtMetrics;
	vec4 uvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D colorTex;
layout(set = 1, binding = 1) uniform sampler2D blendTex;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 offset0;

layout(location = 0) out vec4 FragColor;

vec3 sampleColor(vec2 p)
{
	return texture(colorTex, min(p, uvScaleClamp.zw)).rgb;
}

vec4 sampleBlend(vec2 p)
{
	return texture(blendTex, min(p, uvScaleClamp.zw));
}

void movc2(bvec2 cond, inout vec2 variable, vec2 value)
{
	if (cond.x) { variable.x = value.x; }
	if (cond.y) { variable.y = value.y; }
}

void movc4(bvec4 cond, inout vec4 variable, vec4 value)
{
	if (cond.x) { variable.x = value.x; }
	if (cond.y) { variable.y = value.y; }
	if (cond.z) { variable.z = value.z; }
	if (cond.w) { variable.w = value.w; }
}

void main()
{
	// blending weights covering this pixel, from itself and its right and top neighbors
	vec4 a;
	a.x = sampleBlend(offset0.xy).a;
	a.y = sampleBlend(offset0.zw).g;
	a.wz = sampleBlend(uv).xz;

	if (dot(a, vec4(1.0)) < 1e-5)
	{
		FragColor = vec4(sampleColor(uv), 1.0);
		return;
	}

	// is the dominant blending direction horizontal?
	bool h = max(a.x, a.z) > max(a.y, a.w);

	// blending fractions and normalized weights along that direction
	vec2 f = a.yw;
	movc2(bvec2(h), f, a.xz);
	vec2 blendingWeight = f / dot(f, vec2(1.0));

#ifdef WZ_SMAA_GAMMA2_BLEND
	// the reference implementation mixes each pixel with its neighbor through
	// the hardware bilinear filter, which operates on the stored gamma encoded
	// values and darkens blends across high contrast edges, so fetch the
	// texels directly and mix in approximate linear light instead
	vec2 dir = h ? vec2(rtMetrics.x, 0.0) : vec2(0.0, rtMetrics.y);
	vec3 c0 = sampleColor(uv);
	vec3 c1 = sampleColor(uv + dir);
	vec3 c2 = sampleColor(uv - dir);
	c0 *= c0;
	c1 *= c1;
	c2 *= c2;
	vec3 color = blendingWeight.x * mix(c0, c1, f.x);
	color += blendingWeight.y * mix(c0, c2, f.y);

	FragColor = vec4(sqrt(color), 1.0);
#else
	vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w);
	movc4(bvec4(h), blendingOffset, vec4(a.x, 0.0, a.z, 0.0));
	vec4 blendingCoord = blendingOffset * vec4(rtMetrics.xy, -rtMetrics.xy) + uv.xyxy;

	// bilinear filtering mixes the current pixel with the chosen neighbor
	vec3 color = blendingWeight.x * sampleColor(blendingCoord.xy);
	color += blendingWeight.y * sampleColor(blendingCoord.zw);

	FragColor = vec4(color, 1.0);
#endif
}
