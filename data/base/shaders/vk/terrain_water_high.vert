#version 450

#include "terrain_water_high.glsl"

layout(location = 0) in vec4 vertex; // .w is depth

layout(location = 1) out vec4 uv1_uv2;
layout(location = 2) out vec2 uvLightmap;
layout(location = 3) out float vertexDistance;
layout(location = 4) out vec3 lightDir;
layout(location = 5) out vec3 halfVec;
layout(location = 6) out float depth;
layout(location = 7) out vec3 eyeVec;
layout(location = 8) out float fresnel;
layout(location = 9) out float fresnel_alpha;
layout(location = 10) out FragData frag;

void main()
{
	depth = (vertex.w)/96.0;
	depth = 1.0-(clamp(depth, -1.0, 1.0)*0.5+0.5);

	uvLightmap = (ModelUVLightmapMatrix * vec4(vertex.xyz, 1.f)).xy;

	vec2 uv1 = vec2(vertex.x/3.f/128.f, -vertex.z/3.f/128.f + timeSec/45.f); // (ModelUV1Matrix * vertex).xy;
	vec2 uv2 = vec2(vertex.x/4.f/128.f, vertex.z/4.f/128.f + timeSec/60.f); // (ModelUV2Matrix * vertex).xy;
	uv1_uv2 = vec4(uv1.x, uv1.y, uv2.x, uv2.y);

	eyeVec = normalize(cameraPos.xyz - vertex.xyz);
	lightDir = sunPos.xyz;
	halfVec = normalize(lightDir + eyeVec);

	fresnel = dot(eyeVec, halfVec);
	fresnel = pow(1.0 - fresnel, 10.0)*1000.0;
	fresnel_alpha = dot(eyeVec, vec3(0.0, 1.0, 0.0));
	fresnel_alpha = 1.0-fresnel_alpha;
	fresnel_alpha = pow(fresnel_alpha,0.8);
	fresnel_alpha = clamp(fresnel_alpha,0.15, 0.5);

	frag.fragPos = vertex.xyz;

	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.f);
	vertexDistance = position.z;
	gl_Position = position;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
