#version 120
#pragma debug(on)

void main(void)
{
	// Pass texture coordinates to fragment shader
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	// Pass glColor to fragment shader
	gl_FrontColor = gl_Color;
	
	// Translate every vertex according to the Model View and Projection Matrix
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
