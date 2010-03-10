#pragma debug(on)

#ifdef FOG_ENABLED
varying float vertexDistance;
#endif

void main(void)
{
	// Pass vertex color information to fragment shader
	gl_FrontColor = gl_Color;
	
	// Pass texture coordinates to fragment shader
	gl_TexCoord[0] = gl_TextureMatrix [0] * gl_MultiTexCoord0;

	// Translate every vertex according to the Model View and Projection Matrix
	//gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
	// Use "magic" fixed routine while using GLSL < 1.30
	gl_Position = ftransform();
	
#ifdef FOG_ENABLED
	// Remember vertex distance
	vertexDistance = gl_Position.z;
#endif
}