// Tangent-space helpers
//
// WZ's texture coordinates put v = 0 at the TOP of the image (PNGs are
// uploaded without a vertical flip, and the 2D/atlas/font paths are built on the
// same top-left origin). The tangent basis is therefore arranged so that
// tangent-space +Y points UP the image:
//
//   X  ->  +U in the texture
//   Y  ->  up the image        (so +Y is -V)
//   Z  ->  out of the surface, along the shading normal
//
// That is what makes ordinary "green up" normal maps light correctly, and it is
// why the tangent generator is fed V-flipped coordinates (see imdload.cpp).
//
// The consequence, and the only place this convention leaks into shader code:
// converting a tangent-space DIRECTION into a UV DELTA needs the V component
// negated, because texCoord.y runs the other way. Everything that does that -
// parallax, parallax occlusion, relief mapping, UV flow, anisotropic sampling -
// goes through wzTangentDirToUV() so the sign lives in exactly one place.
//
// Sampling a normal map needs no correction at all: the tangent basis already
// accounts for it.

#ifndef WZ_TANGENTSPACE_GLSL
#define WZ_TANGENTSPACE_GLSL

// Convert a direction expressed in tangent space into a texture-coordinate
// delta. This is the one and only place the v-down convention is applied.
vec2 wzTangentDirToUV(vec2 tangentSpaceDir)
{
	return vec2(tangentSpaceDir.x, -tangentSpaceDir.y);
}

// Classic parallax offset mapping
//
//   tbn        - the (T, B, N) matrix, as built by the vertex shader
//   viewVec    - surface -> eye, in the same space as tbn (i.e. world space)
//   height     - sampled height, 0..1
//   scale      - displacement scale, in UV units
//
// Add the result to texCoord. Write it exactly as the textbook does - the sign
// correction is handled inside.
vec2 wzParallaxOffset(mat3 tbn, vec3 viewVec, float height, float scale)
{
	vec3 v = normalize(transpose(tbn) * viewVec);

	// Guard against grazing angles, where v.z -> 0 blows the offset up.
	float denom = max(abs(v.z), 0.2);

	return wzTangentDirToUV(v.xy / denom) * (height * scale);
}

// Decode a sampled normal map into the space the caller lights in.
//
//   sampled      - the raw texture sample (.xyz, 0..1)
//   hasTangents  - non-zero when the model carries a tangent basis
//   tbn          - the (T, B, N) matrix; pass mat3(1.0) if already lighting in
//                  tangent space, in which case the tangent branch is a no-op
//   normalMatrix - model -> world, used only by the object-space branch
//
// The two branches are NOT symmetric and neither swizzle is redundant:
//
//  - tangent space: the basis is (T, B, N) and the map is (R, G, B) along
//    exactly those axes, so the sample is used unswizzled. See
//    tangentspace.glsl for why no v-flip correction is needed - the tangent
//    generator already accounts for it.
//
//  - object space: the map is in the model's own axes, which WZ stores with y
//    and z exchanged relative to the shader, hence .xzy; and with x and z
//    running the other way, hence the two negations. This path never touches
//    the tangent basis, so nothing here cancels against it.
vec3 wzDecodeNormalMap(vec3 sampled, int hasTangents, mat3 tbn, mat3 normalMatrix)
{
	if (hasTangents != 0)
	{
		return tbn * (sampled.rgb * 2.0 - 1.0);
	}

	vec3 nObjectSpace = sampled.xzy * 2.0 - 1.0;
	return normalMatrix * vec3(-nObjectSpace.x, nObjectSpace.y, -nObjectSpace.z);
}

#endif // WZ_TANGENTSPACE_GLSL
