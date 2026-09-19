// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// FSR 1.0 RCAS (robust contrast adaptive sharpening), fragment shader port.
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

uniform sampler2D Texture;

// con0.x = exp2(-sharpness) per FsrRcasCon, con1.xy = input texel size
layout(std140) uniform cbuffer {
	vec4 con0;
	vec4 con1;
};

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv) texture2D(tex,uv)
#endif

#ifdef NEWGL
in vec2 texCoords;
out vec4 FragColor;
#else
varying vec2 texCoords;
#define FragColor gl_FragColor
#endif

// This uses a 5 tap filter in a '+' pattern (plus shape),
// where the limit is set to the minimal local contrast.
void main()
{
	// Algorithm uses minimal 3x3 pixel neighborhood.
	//    b
	//  d e f
	//    h
	vec2 texelSize = con1.xy;
	vec3 b = texture(Texture, texCoords + vec2( 0.0, -1.0) * texelSize).rgb;
	vec3 d = texture(Texture, texCoords + vec2(-1.0,  0.0) * texelSize).rgb;
	vec3 e = texture(Texture, texCoords                                ).rgb;
	vec3 f = texture(Texture, texCoords + vec2( 1.0,  0.0) * texelSize).rgb;
	vec3 h = texture(Texture, texCoords + vec2( 0.0,  1.0) * texelSize).rgb;
	// Min and max of ring.
	vec3 mn4 = min(min(b, d), min(f, h));
	vec3 mx4 = max(max(b, d), max(f, h));
	// Immediate constants for peak range.
	vec2 peakC = vec2(1.0, -4.0);
	// Limiters, these need to be high precision RCPs.
	// A small epsilon keeps flat black or white regions from producing 0 * inf
	vec3 hitMin = min(mn4, e) / (4.0 * mx4 + vec3(1.0 / 32768.0));
	vec3 hitMax = (vec3(peakC.x) - max(mx4, e)) / (4.0 * mn4 + vec3(peakC.y) - vec3(1.0 / 32768.0));
	vec3 lobeRGB = max(-hitMin, hitMax);
	// FSR_RCAS_LIMIT is the maximum amount of sharpening: (0.25 - (1.0 / 16.0))
	float lobe = max(-(0.25 - (1.0 / 16.0)), min(max(lobeRGB.r, max(lobeRGB.g, lobeRGB.b)), 0.0)) * con0.x;
	// Resolve, which needs the medium precision rcp approximation to avoid visible tonality changes.
	float rcpL = 1.0 / (4.0 * lobe + 1.0);
	vec3 pix = ((b + d + f + h) * lobe + e) * rcpL;

	FragColor = vec4(pix, 1.0);
}
