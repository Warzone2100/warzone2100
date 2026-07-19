// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// SMAA 1x blending weight calculation pass, fragment shader port.
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

// This port targets SMAA 1x, the temporal subsample indices are always zero
// and have been folded out of the area texture lookups.

uniform sampler2D edgesTex;
uniform sampler2D areaTex;
uniform sampler2D searchTex;

// xy = 1 / input size, zw = input size (in physical texels)
uniform vec4 rtMetrics;
// xy scales viewport spanning texcoords down to the rendered sub-rect of the
// input, zw clamps taps just inside its edge (both are identity-like at full size)
uniform vec4 uvScaleClamp;
// x = max orthogonal search steps, y = max diagonal search steps (0 disables
// diagonal processing), z = corner rounding [0..1] (1 disables corner processing)
uniform vec4 params;

#define SMAA_AREATEX_MAX_DISTANCE 16.0
#define SMAA_AREATEX_MAX_DISTANCE_DIAG 20.0
#define SMAA_AREATEX_PIXEL_SIZE (1.0 / vec2(160.0, 560.0))
#define SMAA_SEARCHTEX_SIZE vec2(66.0, 33.0)
#define SMAA_SEARCHTEX_PACKED_SIZE vec2(64.0, 16.0)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv) texture2D(tex,uv)
#define round(x) floor((x) + 0.5)
#endif

#ifdef NEWGL
in vec2 uv;
in vec2 pixcoord;
in vec4 offset0;
in vec4 offset1;
in vec4 offset2;
out vec4 FragColor;
#else
varying vec2 uv;
varying vec2 pixcoord;
varying vec4 offset0;
varying vec4 offset1;
varying vec4 offset2;
#define FragColor gl_FragColor
#endif

vec2 sampleEdges(vec2 p)
{
	return texture(edgesTex, min(p, uvScaleClamp.zw)).rg;
}

vec2 sampleEdgesOffset(vec2 p, vec2 texelOffset)
{
	return sampleEdges(p + texelOffset * rtMetrics.xy);
}

// unpacks two binary edge values from a bilinear fetch between texels
vec2 decodeDiagBilinearAccess2(vec2 e)
{
	e.r = e.r * abs(5.0 * e.r - 5.0 * 0.75);
	return round(e);
}

vec4 decodeDiagBilinearAccess4(vec4 e)
{
	e.rb = e.rb * abs(5.0 * e.rb - 5.0 * 0.75);
	return round(e);
}

void movc2(bvec2 cond, inout vec2 variable, vec2 value)
{
	if (cond.x) { variable.x = value.x; }
	if (cond.y) { variable.y = value.y; }
}

vec2 searchDiag1(vec2 texcoord, vec2 dir, out vec2 e)
{
	vec4 coord = vec4(texcoord, -1.0, 1.0);
	vec3 t = vec3(rtMetrics.xy, 1.0);
	e = vec2(0.0);
	while (coord.z < params.y - 1.0 && coord.w > 0.9)
	{
		coord.xyz = t * vec3(dir, 1.0) + coord.xyz;
		e = sampleEdges(coord.xy);
		coord.w = dot(e, vec2(0.5));
	}
	return coord.zw;
}

vec2 searchDiag2(vec2 texcoord, vec2 dir, out vec2 e)
{
	vec4 coord = vec4(texcoord, -1.0, 1.0);
	coord.x += 0.25 * rtMetrics.x;
	vec3 t = vec3(rtMetrics.xy, 1.0);
	e = vec2(0.0);
	while (coord.z < params.y - 1.0 && coord.w > 0.9)
	{
		coord.xyz = t * vec3(dir, 1.0) + coord.xyz;

		// fetch both edge values at once with a bilinear tap between texels
		e = sampleEdges(coord.xy);
		e = decodeDiagBilinearAccess2(e);

		coord.w = dot(e, vec2(0.5));
	}
	return coord.zw;
}

// area for a diagonal distance and crossing edges pair
vec2 areaDiag(vec2 dist, vec2 e)
{
	vec2 coord = SMAA_AREATEX_MAX_DISTANCE_DIAG * e + dist;

	// scale and bias for mapping to texel space
	coord = SMAA_AREATEX_PIXEL_SIZE * coord + 0.5 * SMAA_AREATEX_PIXEL_SIZE;

	// diagonal areas are on the second half of the texture
	coord.x += 0.5;

	return texture(areaTex, coord).rg;
}

// searches for diagonal patterns around the pixel and returns their weights
vec2 calculateDiagWeights(vec2 texcoord, vec2 e)
{
	vec2 weights = vec2(0.0);

	// search for the line ends
	vec4 d;
	vec2 end;
	if (e.r > 0.0)
	{
		d.xz = searchDiag1(texcoord, vec2(-1.0, 1.0), end);
		d.x += float(end.y > 0.9);
	}
	else
	{
		d.xz = vec2(0.0);
	}
	d.yw = searchDiag1(texcoord, vec2(1.0, -1.0), end);

	if (d.x + d.y > 2.0)
	{
		// fetch the crossing edges
		vec4 coords = vec4(-d.x + 0.25, d.x, d.y, -d.y - 0.25) * rtMetrics.xyxy + texcoord.xyxy;
		vec4 c;
		c.xy = sampleEdgesOffset(coords.xy, vec2(-1.0, 0.0));
		c.zw = sampleEdgesOffset(coords.zw, vec2( 1.0, 0.0));
		c.yxwz = decodeDiagBilinearAccess4(c.xyzw);

		// merge crossing edges at each side into a single value
		vec2 cc = vec2(2.0) * c.xz + c.yw;

		// remove the crossing edge when the end of the line was not found
		movc2(bvec2(step(vec2(0.9), d.zw)), cc, vec2(0.0));

		weights += areaDiag(d.xy, cc);
	}

	// search for the line ends of the other diagonal
	d.xz = searchDiag2(texcoord, vec2(-1.0, -1.0), end);
	if (sampleEdgesOffset(texcoord, vec2(1.0, 0.0)).r > 0.0)
	{
		d.yw = searchDiag2(texcoord, vec2(1.0, 1.0), end);
		d.y += float(end.y > 0.9);
	}
	else
	{
		d.yw = vec2(0.0);
	}

	if (d.x + d.y > 2.0)
	{
		// fetch the crossing edges
		vec4 coords = vec4(-d.x, -d.x, d.y, d.y) * rtMetrics.xyxy + texcoord.xyxy;
		vec4 c;
		c.x  = sampleEdgesOffset(coords.xy, vec2(-1.0, 0.0)).g;
		c.y  = sampleEdgesOffset(coords.xy, vec2(0.0, -1.0)).r;
		c.zw = sampleEdgesOffset(coords.zw, vec2(1.0, 0.0)).gr;
		vec2 cc = vec2(2.0) * c.xz + c.yw;

		movc2(bvec2(step(vec2(0.9), d.zw)), cc, vec2(0.0));

		weights += areaDiag(d.xy, cc).gr;
	}

	return weights;
}

// determines how much length to add in the last step of the searches, from
// the bilinearly packed edge value and the crossing edges
float searchLength(vec2 e, float bias)
{
	// the search texture is flipped vertically and packs the left and right
	// cases side by side
	vec2 scale = SMAA_SEARCHTEX_SIZE * vec2(0.5, -1.0);
	vec2 offset = SMAA_SEARCHTEX_SIZE * vec2(bias, 1.0);

	// scale and bias to access texel centers
	scale += vec2(-1.0, 1.0);
	offset += vec2(0.5, -0.5);

	// convert from pixel coordinates to texcoords
	scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
	offset *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;

	return texture(searchTex, scale * e + offset).r;
}

float searchXLeft(vec2 texcoord, float end)
{
	// the texcoord was offset in the vertex shader to sample between edges,
	// fetching four edge values per bilinear tap
	vec2 e = vec2(0.0, 1.0);
	while (texcoord.x > end && e.g > 0.8281 && e.r == 0.0)
	{
		e = sampleEdges(texcoord);
		texcoord = -vec2(2.0, 0.0) * rtMetrics.xy + texcoord;
	}
	float offset = -(255.0 / 127.0) * searchLength(e, 0.0) + 3.25;
	return rtMetrics.x * offset + texcoord.x;
}

float searchXRight(vec2 texcoord, float end)
{
	vec2 e = vec2(0.0, 1.0);
	while (texcoord.x < end && e.g > 0.8281 && e.r == 0.0)
	{
		e = sampleEdges(texcoord);
		texcoord = vec2(2.0, 0.0) * rtMetrics.xy + texcoord;
	}
	float offset = -(255.0 / 127.0) * searchLength(e, 0.5) + 3.25;
	return -rtMetrics.x * offset + texcoord.x;
}

float searchYUp(vec2 texcoord, float end)
{
	vec2 e = vec2(1.0, 0.0);
	while (texcoord.y > end && e.r > 0.8281 && e.g == 0.0)
	{
		e = sampleEdges(texcoord);
		texcoord = -vec2(0.0, 2.0) * rtMetrics.xy + texcoord;
	}
	float offset = -(255.0 / 127.0) * searchLength(e.gr, 0.0) + 3.25;
	return rtMetrics.y * offset + texcoord.y;
}

float searchYDown(vec2 texcoord, float end)
{
	vec2 e = vec2(1.0, 0.0);
	while (texcoord.y < end && e.r > 0.8281 && e.g == 0.0)
	{
		e = sampleEdges(texcoord);
		texcoord = vec2(0.0, 2.0) * rtMetrics.xy + texcoord;
	}
	float offset = -(255.0 / 127.0) * searchLength(e.gr, 0.5) + 3.25;
	return -rtMetrics.y * offset + texcoord.y;
}

// area at each side of the edge for the distance and crossing edges pair
vec2 area(vec2 dist, float e1, float e2)
{
	// rounding prevents precision errors of bilinear filtering
	vec2 coord = SMAA_AREATEX_MAX_DISTANCE * round(4.0 * vec2(e1, e2)) + dist;

	// scale and bias for mapping to texel space
	coord = SMAA_AREATEX_PIXEL_SIZE * coord + 0.5 * SMAA_AREATEX_PIXEL_SIZE;

	return texture(areaTex, coord).rg;
}

void detectHorizontalCornerPattern(inout vec2 weights, vec4 texcoord, vec2 d)
{
	vec2 leftRight = step(d.xy, d.yx);
	vec2 rounding = (1.0 - params.z) * leftRight;

	// reduce blending for pixels in the center of a line
	rounding /= leftRight.x + leftRight.y;

	vec2 factor = vec2(1.0);
	factor.x -= rounding.x * sampleEdgesOffset(texcoord.xy, vec2(0.0,  1.0)).r;
	factor.x -= rounding.y * sampleEdgesOffset(texcoord.zw, vec2(1.0,  1.0)).r;
	factor.y -= rounding.x * sampleEdgesOffset(texcoord.xy, vec2(0.0, -2.0)).r;
	factor.y -= rounding.y * sampleEdgesOffset(texcoord.zw, vec2(1.0, -2.0)).r;

	weights *= clamp(factor, 0.0, 1.0);
}

void detectVerticalCornerPattern(inout vec2 weights, vec4 texcoord, vec2 d)
{
	vec2 leftRight = step(d.xy, d.yx);
	vec2 rounding = (1.0 - params.z) * leftRight;

	rounding /= leftRight.x + leftRight.y;

	vec2 factor = vec2(1.0);
	factor.x -= rounding.x * sampleEdgesOffset(texcoord.xy, vec2( 1.0, 0.0)).g;
	factor.x -= rounding.y * sampleEdgesOffset(texcoord.zw, vec2( 1.0, 1.0)).g;
	factor.y -= rounding.x * sampleEdgesOffset(texcoord.xy, vec2(-2.0, 0.0)).g;
	factor.y -= rounding.y * sampleEdgesOffset(texcoord.zw, vec2(-2.0, 1.0)).g;

	weights *= clamp(factor, 0.0, 1.0);
}

void main()
{
	vec4 weights = vec4(0.0);

	vec2 e = sampleEdges(uv);

	if (e.g > 0.0)
	{
		// edge at north
		bool doOrthogonal = true;
		if (params.y > 0.0)
		{
			// diagonals have both north and west edges, give them priority
			weights.rg = calculateDiagWeights(uv, e);
			doOrthogonal = (weights.r + weights.g == 0.0);
		}
		if (doOrthogonal)
		{
			vec2 d;

			// distance to the left
			vec3 coords;
			coords.x = searchXLeft(offset0.xy, offset2.x);
			coords.y = offset1.y;
			d.x = coords.x;

			// fetch the left crossing edges, two at a time with bilinear filtering
			float e1 = sampleEdges(coords.xy).r;

			// distance to the right
			coords.z = searchXRight(offset0.zw, offset2.y);
			d.y = coords.z;

			// convert the distances to pixel units
			d = abs(round(rtMetrics.zz * d - pixcoord.xx));

			// the areas texture is compressed quadratically
			vec2 sqrt_d = sqrt(d);

			// fetch the right crossing edges
			float e2 = sampleEdgesOffset(vec2(coords.z, coords.y), vec2(1.0, 0.0)).r;

			vec2 w = area(sqrt_d, e1, e2);

			// fix corners
			coords.y = uv.y;
			if (params.z < 1.0)
			{
				detectHorizontalCornerPattern(w, vec4(coords.xy, coords.z, coords.y), d);
			}
			weights.rg = w;
		}
		else
		{
			// a diagonal was found, skip vertical processing
			e.r = 0.0;
		}
	}

	if (e.r > 0.0)
	{
		// edge at west
		vec2 d;

		// distance to the top
		vec3 coords;
		coords.y = searchYUp(offset1.xy, offset2.z);
		coords.x = offset0.x;
		d.x = coords.y;

		// fetch the top crossing edges
		float e1 = sampleEdges(coords.xy).g;

		// distance to the bottom
		coords.z = searchYDown(offset1.zw, offset2.w);
		d.y = coords.z;

		// convert the distances to pixel units
		d = abs(round(rtMetrics.ww * d - pixcoord.yy));

		// the areas texture is compressed quadratically
		vec2 sqrt_d = sqrt(d);

		// fetch the bottom crossing edges
		float e2 = sampleEdgesOffset(vec2(coords.x, coords.z), vec2(0.0, 1.0)).g;

		vec2 w = area(sqrt_d, e1, e2);

		// fix corners
		coords.x = uv.x;
		if (params.z < 1.0)
		{
			detectVerticalCornerPattern(w, vec4(coords.xy, coords.x, coords.z), d);
		}
		weights.ba = w;
	}

	FragColor = weights;
}
