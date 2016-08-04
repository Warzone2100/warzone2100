#version 120
#pragma debug(on)

uniform float stretch;

varying float vertexDistance;
varying vec3 normal, lightDir, eyeVec;

void main(void)
{
	vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
	vec4 position = gl_Vertex;

	// Pass texture coordinates to fragment shader
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	// Lighting -- we pass these to the fragment shader
	normal = gl_NormalMatrix * gl_Normal;
	lightDir = vec3(gl_LightSource[0].position.xyz - vVertex);
	eyeVec = -vVertex;
	gl_FrontColor = gl_Color;

	// Implement building stretching to accomodate terrain
	if (gl_Vertex.y <= 0.0) // use gl_Vertex here directly to help shader compiler optimization
	{
		position.y -= stretch;
	}
	
	// Translate every vertex according to the Model View and Projection Matrix
	gl_Position = gl_ModelViewProjectionMatrix * position;

	// Remember vertex distance
	vertexDistance = gl_Position.z;
}
