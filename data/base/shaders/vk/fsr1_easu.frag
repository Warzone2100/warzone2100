#version 450

// FSR 1.0 EASU (edge adaptive spatial upsampling), fragment shader port.
// Based on AMD FidelityFX Super Resolution 1.0 (ffx_a.h / ffx_fsr1.h)
//
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// standard FsrEasuCon constants
// con0.xy = input size / output size, con0.zw = 0.5 * con0.xy - 0.5
// con1.xy = input texel size (con1.zw, con2, con3 exist for the gather path and are unused here)
layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 con0;
	vec4 con1;
	vec4 con2;
	vec4 con3;
};

layout(set = 1, binding = 0) uniform sampler2D Texture;

layout(location = 0) out vec4 FragColor;

// Filtering for a given tap for the scalar.
void fsrEasuTap(
	inout vec3 aC, // Accumulated color, with negative lobe.
	inout float aW, // Accumulated weight.
	vec2 off, // Pixel offset from resolve position to tap.
	vec2 dir, // Gradient direction.
	vec2 len, // Length.
	float lob, // Negative lobe strength.
	float clp, // Clipping point.
	vec3 c) // Tap color.
{
	// Rotate offset by direction.
	vec2 v;
	v.x = (off.x * ( dir.x)) + (off.y * dir.y);
	v.y = (off.x * (-dir.y)) + (off.y * dir.x);
	// Anisotropy.
	v *= len;
	// Compute distance^2.
	float d2 = v.x * v.x + v.y * v.y;
	// Limit to the window as at corner, 2 taps can easily be outside.
	d2 = min(d2, clp);
	// Approximation of lanczos2 without sin() or rcp(), or sqrt() to get x.
	//  (25/16 * (2/5 * x^2 - 1)^2 - (25/16 - 1)) * (1/4 * x^2 - 1)^2
	float wB = (2.0 / 5.0) * d2 - 1.0;
	float wA = lob * d2 - 1.0;
	wB *= wB;
	wA *= wA;
	wB = (25.0 / 16.0) * wB - (25.0 / 16.0 - 1.0);
	float w = wB * wA;
	// Do weighted average.
	aC += c * w;
	aW += w;
}

// Accumulate direction and length.
void fsrEasuSet(
	inout vec2 dir,
	inout float len,
	float w, // Bilinear weight for this corner.
	float lA, float lB, float lC, float lD, float lE) // Lumas of the cross around the corner.
{
	// Direction is the '+' diff.
	//    a
	//  b c d
	//    e
	// Length is the reciprocal of the contrast, using a bounded reciprocal
	// in place of the reference's approximate reciprocal to avoid inf * 0
	float dc = lD - lC;
	float cb = lC - lB;
	float lenX = max(abs(dc), abs(cb));
	lenX = 1.0 / (lenX + (1.0 / 32768.0));
	float dirX = lD - lB;
	dir.x += dirX * w;
	lenX = clamp(abs(dirX) * lenX, 0.0, 1.0);
	lenX *= lenX;
	len += lenX * w;
	// Repeat for the y axis.
	float ec = lE - lC;
	float ca = lC - lA;
	float lenY = max(abs(ec), abs(ca));
	lenY = 1.0 / (lenY + (1.0 / 32768.0));
	float dirY = lE - lA;
	dir.y += dirY * w;
	lenY = clamp(abs(dirY) * lenY, 0.0, 1.0);
	lenY *= lenY;
	len += lenY * w;
}

float fsrLuma(vec3 c)
{
	return 0.5 * c.b + 0.5 * c.r + c.g;
}

void main()
{
	//------------------------------------------------------------------------------------------------------------------------------
	//      +---+---+
	//      |   |   |
	//      +--(0)--+
	//      | b | c |
	//  +---F---+---+---+
	//  | e | f | g | h |
	//  +--(1)--+--(2)--+
	//  | i | j | k | l |
	//  +---+---+---+---+
	//      | n | o |
	//      +--(3)--+
	//      |   |   |
	//      +---+---+
	// Get position of 'f'.
	vec2 pp = floor(gl_FragCoord.xy) * con0.xy + con0.zw;
	vec2 fp = floor(pp);
	pp -= fp;
	//------------------------------------------------------------------------------------------------------------------------------
	// The 12 tap colors (tap centers relative to 'f').
	vec2 texelSize = con1.xy;
	vec2 basePos = (fp + vec2(0.5, 0.5)) * texelSize;
	vec3 cB = texture(Texture, basePos + vec2( 0.0, -1.0) * texelSize).rgb;
	vec3 cC = texture(Texture, basePos + vec2( 1.0, -1.0) * texelSize).rgb;
	vec3 cE = texture(Texture, basePos + vec2(-1.0,  0.0) * texelSize).rgb;
	vec3 cF = texture(Texture, basePos                                ).rgb;
	vec3 cG = texture(Texture, basePos + vec2( 1.0,  0.0) * texelSize).rgb;
	vec3 cH = texture(Texture, basePos + vec2( 2.0,  0.0) * texelSize).rgb;
	vec3 cI = texture(Texture, basePos + vec2(-1.0,  1.0) * texelSize).rgb;
	vec3 cJ = texture(Texture, basePos + vec2( 0.0,  1.0) * texelSize).rgb;
	vec3 cK = texture(Texture, basePos + vec2( 1.0,  1.0) * texelSize).rgb;
	vec3 cL = texture(Texture, basePos + vec2( 2.0,  1.0) * texelSize).rgb;
	vec3 cN = texture(Texture, basePos + vec2( 0.0,  2.0) * texelSize).rgb;
	vec3 cO = texture(Texture, basePos + vec2( 1.0,  2.0) * texelSize).rgb;
	//------------------------------------------------------------------------------------------------------------------------------
	// Simplest multi-channel approximate luma possible (luma times 2, in 2 FMA/MAD).
	float lB = fsrLuma(cB);
	float lC = fsrLuma(cC);
	float lE = fsrLuma(cE);
	float lF = fsrLuma(cF);
	float lG = fsrLuma(cG);
	float lH = fsrLuma(cH);
	float lI = fsrLuma(cI);
	float lJ = fsrLuma(cJ);
	float lK = fsrLuma(cK);
	float lL = fsrLuma(cL);
	float lN = fsrLuma(cN);
	float lO = fsrLuma(cO);
	// Accumulate for bilinear interpolation.
	vec2 dir = vec2(0.0);
	float len = 0.0;
	fsrEasuSet(dir, len, (1.0 - pp.x) * (1.0 - pp.y), lB, lE, lF, lG, lJ);
	fsrEasuSet(dir, len,        pp.x  * (1.0 - pp.y), lC, lF, lG, lH, lK);
	fsrEasuSet(dir, len, (1.0 - pp.x) *        pp.y , lF, lI, lJ, lK, lN);
	fsrEasuSet(dir, len,        pp.x  *        pp.y , lG, lJ, lK, lL, lO);
	//------------------------------------------------------------------------------------------------------------------------------
	// Normalize with approximation, and cleanup close to zero.
	vec2 dir2 = dir * dir;
	float dirR = dir2.x + dir2.y;
	bool zro = dirR < (1.0 / 32768.0);
	dirR = inversesqrt(dirR);
	dirR = zro ? 1.0 : dirR;
	dir.x = zro ? 1.0 : dir.x;
	dir *= vec2(dirR);
	// Transform from {0 to 2} to {0 to 1} range, and shape with square.
	len = len * 0.5;
	len *= len;
	// Stretch kernel {1.0 vert|horz, to sqrt(2.0) on diagonal}.
	float stretch = (dir.x * dir.x + dir.y * dir.y) * (1.0 / max(abs(dir.x), abs(dir.y)));
	// Anisotropic length after rotation,
	//  x := 1.0 lerp to 'stretch' on edges
	//  y := 1.0 lerp to 2^-1.0 on edges
	vec2 len2 = vec2(1.0 + (stretch - 1.0) * len, 1.0 - 0.5 * len);
	// Based on the amount of 'edge',
	// the window shifts from +/-{sqrt(2.0) to slightly beyond 2.0}.
	float lob = 0.5 + ((1.0 / 4.0 - 0.04) - 0.5) * len;
	// Set distance^2 clipping point to the end of the adjustable window.
	float clp = 1.0 / lob;
	//------------------------------------------------------------------------------------------------------------------------------
	// Accumulation mixed with min/max of 4 nearest.
	//    b c
	//  e f g h
	//  i j k l
	//    n o
	vec3 min4 = min(min(cF, cG), min(cJ, cK));
	vec3 max4 = max(max(cF, cG), max(cJ, cK));
	// Accumulation.
	vec3 aC = vec3(0.0);
	float aW = 0.0;
	fsrEasuTap(aC, aW, vec2( 0.0, -1.0) - pp, dir, len2, lob, clp, cB);
	fsrEasuTap(aC, aW, vec2( 1.0, -1.0) - pp, dir, len2, lob, clp, cC);
	fsrEasuTap(aC, aW, vec2(-1.0,  0.0) - pp, dir, len2, lob, clp, cE);
	fsrEasuTap(aC, aW, vec2( 0.0,  0.0) - pp, dir, len2, lob, clp, cF);
	fsrEasuTap(aC, aW, vec2( 1.0,  0.0) - pp, dir, len2, lob, clp, cG);
	fsrEasuTap(aC, aW, vec2( 2.0,  0.0) - pp, dir, len2, lob, clp, cH);
	fsrEasuTap(aC, aW, vec2(-1.0,  1.0) - pp, dir, len2, lob, clp, cI);
	fsrEasuTap(aC, aW, vec2( 0.0,  1.0) - pp, dir, len2, lob, clp, cJ);
	fsrEasuTap(aC, aW, vec2( 1.0,  1.0) - pp, dir, len2, lob, clp, cK);
	fsrEasuTap(aC, aW, vec2( 2.0,  1.0) - pp, dir, len2, lob, clp, cL);
	fsrEasuTap(aC, aW, vec2( 0.0,  2.0) - pp, dir, len2, lob, clp, cN);
	fsrEasuTap(aC, aW, vec2( 1.0,  2.0) - pp, dir, len2, lob, clp, cO);
	//------------------------------------------------------------------------------------------------------------------------------
	// Normalize and dering.
	vec3 pix = min(max4, max(min4, aC * vec3(1.0 / aW)));

	FragColor = vec4(pix, 1.0);
}
