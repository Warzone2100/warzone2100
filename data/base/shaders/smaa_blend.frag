// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

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

uniform sampler2D colorTex;
uniform sampler2D blendTex;

// xy = 1 / input size, zw = input size (in physical texels)
uniform vec4 rtMetrics;
// xy scales viewport spanning texcoords down to the rendered sub-rect of the
// input, zw clamps taps just inside its edge (both are identity-like at full size)
uniform vec4 uvScaleClamp;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv) texture2D(tex,uv)
#endif

#ifdef NEWGL
in vec2 uv;
in vec4 offset0;
out vec4 FragColor;
#else
varying vec2 uv;
varying vec4 offset0;
#define FragColor gl_FragColor
#endif

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

	// blending offsets and weights along that direction
	vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w);
	vec2 blendingWeight = a.yw;
	movc4(bvec4(h), blendingOffset, vec4(a.x, 0.0, a.z, 0.0));
	movc2(bvec2(h), blendingWeight, a.xz);
	blendingWeight /= dot(blendingWeight, vec2(1.0));

	vec4 blendingCoord = blendingOffset * vec4(rtMetrics.xy, -rtMetrics.xy) + uv.xyxy;

	// bilinear filtering mixes the current pixel with the chosen neighbor
	vec3 color = blendingWeight.x * sampleColor(blendingCoord.xy);
	color += blendingWeight.y * sampleColor(blendingCoord.zw);

	FragColor = vec4(color, 1.0);
}
