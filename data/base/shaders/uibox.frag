// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// Styled UI box: gradient fill with shaped falloff and ordered dithering,
// plus optional SDF rounded corners and border, in a single pass.

uniform vec2 size;				// quad size in pixels
uniform vec2 gradientDir;		// (0,1)=vertical, (1,0)=horizontal, (0,0)=flat colorA
uniform vec4 colorA;			// fill at gradient start
uniform vec4 colorB;			// fill at gradient end
uniform vec4 borderColor;
uniform float gradientExponent;	// 1.0 = linear, 2.0 = (1-t)^2 falloff from colorA
uniform vec4 cornerRadii;		// px per corner: (topLeft, topRight, bottomLeft, bottomRight) - 0 = sharp
uniform vec4 clipRect;			// visible region in quad-local pixels (x0, y0, x1, y1) - x1 <= x0 = no clipping
uniform float borderWidth;		// pixels, 0 = none
uniform float edgeSoftness;		// alpha fade width at the SDF edge, pixels (1.0 = crisp AA)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec2 uv;
out vec4 FragColor;
#else
varying vec2 uv;
#endif

void main()
{
	// Gradient: shaped so that with exponent e, the blend weight toward colorA
	// is exactly (1-t)^e - i.e. e=2 reproduces a quadratic "175*(1-t)^2" falloff.
	float t = clamp(dot(uv, gradientDir), 0.0, 1.0);
	t = 1.0 - pow(1.0 - t, gradientExponent);
	vec4 fill = mix(colorA, colorB, t);

	// Ordered dither (+-0.5/255, incl. alpha) to prevent mach banding on wide,
	// dark gradients in 8-bit render targets.
	float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
	fill += vec4((noise - 0.5) * (1.0 / 255.0));

	// Signed distance to a rounded rectangle, in pixels (negative = inside),
	// with a per-corner radius selected by this fragment's quadrant
	// (uv origin is the top-left corner, so p.y < 0 is the top half)
	vec2 p = uv * size - size * 0.5;
	float cornerRadius = (p.x < 0.0) ? ((p.y < 0.0) ? cornerRadii.x : cornerRadii.z)
	                                 : ((p.y < 0.0) ? cornerRadii.y : cornerRadii.w);
	vec2 q = abs(p) - (size * 0.5 - vec2(cornerRadius, cornerRadius));
	float dist = length(max(q, vec2(0.0, 0.0))) + min(max(q.x, q.y), 0.0) - cornerRadius;

	vec4 shaded = fill;
	if (borderWidth > 0.0)
	{
		float borderMix = clamp(dist + borderWidth + 0.5, 0.0, 1.0);
		shaded = mix(fill, borderColor, borderMix);
	}
	// Anti-aliased edge - widths > 1px give a soft, blurred-looking rim.
	// Soft rims are smoothstepped: a linear ramp's start/end derivative
	// kinks read as harsh edges at wide widths. Crisp (<= 1px) AA edges
	// keep the plain linear ramp.
	float edgeT = clamp((0.5 - dist) / max(edgeSoftness, 1.0), 0.0, 1.0);
	if (edgeSoftness > 1.0)
	{
		edgeT = edgeT * edgeT * (3.0 - 2.0 * edgeT);
	}
	shaded.a *= edgeT;

	// Optional clip: cut the (full) shape at the clip rect's edges, with a
	// crisp 1px AA cut - partially visible rounded corners render as
	// truncated arcs, not reshaped ones
	if (clipRect.z > clipRect.x && clipRect.w > clipRect.y)
	{
		vec2 pos = uv * size;
		vec2 clipCenter = (clipRect.xy + clipRect.zw) * 0.5;
		vec2 clipHalfExtent = (clipRect.zw - clipRect.xy) * 0.5;
		vec2 cq = abs(pos - clipCenter) - clipHalfExtent;
		float clipDist = length(max(cq, vec2(0.0, 0.0))) + min(max(cq.x, cq.y), 0.0);
		shaded.a *= clamp(0.5 - clipDist, 0.0, 1.0);
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = shaded;
	#else
	gl_FragColor = shaded;
	#endif
}
