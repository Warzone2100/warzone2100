// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 ModelViewProjectionMatrix;

uniform vec4 paramxlight;
uniform vec4 paramylight;
uniform mat4 lightTextureMatrix;

#if __VERSION__ >= 130
in vec4 vertex;
in vec2 vertexTexCoord;
#else
attribute vec4 vertex;
attribute vec2 vertexTexCoord;
#endif

#if __VERSION__ >= 130
out vec2 uv_tex;
out vec2 uv_lightmap;
out float vertexDistance;
#else
varying vec2 uv_tex;
varying vec2 uv_lightmap;
varying float vertexDistance;
#endif

void main()
{
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	uv_tex = vertexTexCoord;
	vec4 uv_lightmap_tmp = lightTextureMatrix * vec4(dot(paramxlight, vertex), dot(paramylight, vertex), 0.0, 1.0);
	uv_lightmap = uv_lightmap_tmp.xy / uv_lightmap_tmp.w;
	vertexDistance = position.z;
}
