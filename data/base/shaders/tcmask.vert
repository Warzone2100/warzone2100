#version 120
#pragma debug(on)

varying float vertexDistance;
varying vec3 normal;

uniform float stretch;
uniform int fogEnabled;

void main(void)
{
	vec3 lightDir;
	vec4 diffuse, ambient, globalAmbient;
	float NdotL;
	vec4 position = gl_Vertex;

	// Pass texture coordinates to fragment shader
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	// Lighting
	normal = normalize(gl_NormalMatrix * gl_Normal);
	lightDir = normalize(vec3(gl_LightSource[0].position));
	NdotL = max(dot(normal, lightDir), 0.0);
	diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
	ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
	globalAmbient = gl_LightModel.ambient * gl_FrontMaterial.ambient;
	gl_FrontColor =  (NdotL * diffuse + globalAmbient + ambient) * gl_Color;

	if (position.y <= 0.0)
	{
		position.y -= stretch;
	}
	
	// Translate every vertex according to the Model View and Projection Matrix
	gl_Position = gl_ModelViewProjectionMatrix * position;

	if (fogEnabled > 0)
	{
		// Remember vertex distance
		vertexDistance = gl_Position.z;
	}
}
