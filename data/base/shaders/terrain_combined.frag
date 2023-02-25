// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// Texture arrays are supported in:
// 1. core OpenGL 3.0+ (and earlier with EXT_texture_array)
//#if (!defined(GL_ES) && (__VERSION__ < 130))
//#extension GL_EXT_texture_array : require // only enable this on OpenGL < 3.0
//#endif
// 2. OpenGL ES 3.0+
#if (defined(GL_ES) && (__VERSION__ < 300))
#error "Unsupported version of GLES"
#endif

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
//

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#define texture2DArray(tex,coord,bias) texture(tex,coord,bias)
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#endif

uniform sampler2D lightmap_tex;

// ground texture arrays. layer = ground type
uniform sampler2DArray groundTex;
uniform sampler2DArray groundNormal;
uniform sampler2DArray groundSpecular;
uniform sampler2DArray groundHeight;

// array of scales for ground textures, encoded in mat4. scale_i = groundScale[i/4][i%4]
uniform mat4 groundScale;

// decal texture arrays. layer = decal tile
uniform sampler2DArray decalTex;
uniform sampler2DArray decalNormal;
uniform sampler2DArray decalSpecular;
uniform sampler2DArray decalHeight;

// sun light colors/intensity:
uniform vec4 emissiveLight;
uniform vec4 ambientLight;
uniform vec4 diffuseLight;
uniform vec4 specularLight;

// fog
uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

uniform int quality; // 0-classic, 1-bumpmapping

in vec2 uvLightmap;
in vec2 uvDecal;
in vec2 uvGround;
in float vertexDistance;
flat in int tile;
flat in uvec4 fgrounds;
in vec4 fgroundWeights;
// Light in tangent space:
in vec3 groundLightDir;
in vec3 groundHalfVec;
in mat2 decal2groundMat2;

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

vec3 getGroundUv(int i) {
	uint groundNo = fgrounds[i];
	return vec3(uvGround * groundScale[groundNo/4u][groundNo%4u], groundNo);
}

vec3 getGround(int i) {
	return texture2DArray(groundTex, getGroundUv(i), WZ_MIP_LOAD_BIAS).rgb * fgroundWeights[i];
}

vec4 main_classic() {
	vec3 ground = getGround(0) + getGround(1) + getGround(2) + getGround(3);
	vec4 decal = tile > 0 ? texture2DArray(decalTex, vec3(uvDecal, tile), WZ_MIP_LOAD_BIAS) : vec4(0);
	vec4 light = texture(lightmap_tex, uvLightmap, 0.f);
	return light * vec4((1-decal.a) * ground + decal.a * decal.rgb, 1);
}

struct BumpData {
	vec4 color;
	vec3 N;
	float gloss;
};

void getGroundBM(int i, inout BumpData res) {
	vec3 uv = getGroundUv(i);
	float w = fgroundWeights[i];
	res.color += texture2DArray(groundTex, uv, WZ_MIP_LOAD_BIAS) * w;
	vec3 N = texture2DArray(groundNormal, uv, WZ_MIP_LOAD_BIAS).xyz;
	if (N == vec3(0)) {
		N = vec3(0,0,1);
	} else {
		N = normalize(N * 2 - 1);
	}
	res.N += N * w;
	res.gloss += texture2DArray(groundSpecular, uv, WZ_MIP_LOAD_BIAS).r * w;
}

vec4 doBumpMapping(BumpData b, vec3 lightDir, vec3 halfVec) {
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(b.N, L), 0.0); // diffuse lighting
	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float exponent = acos(dot(H, b.N)) / 0.33;
	float gaussianTerm = exp(-(exponent * exponent));

	vec4 res = b.color*(ambientLight*0.5 + diffuseLight*lambertTerm) + specularLight*b.gloss*gaussianTerm*lambertTerm;

	return vec4(res.rgb, b.color.a);
}

vec4 main_bumpMapping() {
	BumpData bump;
	bump.color = vec4(0);
	bump.N = vec3(0);
	bump.gloss = 0;
	getGroundBM(0, bump);
	getGroundBM(1, bump);
	getGroundBM(2, bump);
	getGroundBM(3, bump);

	if (tile > 0) {
		vec3 uv = vec3(uvDecal, tile);
		vec4 decalColor = texture2DArray(decalTex, uv, WZ_MIP_LOAD_BIAS);
		float a = decalColor.a;
		// blend color, normal and gloss with ground ones based on alpha
		bump.color = (1-a)*bump.color + a*vec4(decalColor.rgb, 1);
		vec3 n = texture2DArray(decalNormal, uv, WZ_MIP_LOAD_BIAS).xyz;
		if (n == vec3(0)) {
			n = vec3(0,0,1);
		} else {
			n = normalize(n * 2 - 1);
			n = vec3(n.xy * decal2groundMat2, n.z);
		}
		bump.N = (1-a)*bump.N + a*n;
		bump.gloss = (1-a)*bump.gloss + a*texture2DArray(decalSpecular, uv, WZ_MIP_LOAD_BIAS).r;
	}
	vec4 lightMask = texture(lightmap_tex, uvLightmap, 0.f);
	return lightMask * doBumpMapping(bump, groundLightDir, groundHalfVec);
}

void main()
{
	vec4 fragColor;
	if (quality == 1) {
		fragColor = main_bumpMapping();
	} else {
		fragColor = main_classic();
	}

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), fogFactor);
	}

	#ifdef NEWGL
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
