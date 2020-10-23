#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec2 TexCoord;
in float T;
#else
varying vec2 TexCoord;
varying float T;
#endif

uniform sampler2D tex;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main(void) {
    vec4 sample = texture(tex, TexCoord);
    float opacity = sample.w * (1 - T);
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
    FragColor=vec4(sample.xyz, opacity);
	#else
    gl_FragColor=vec4(sample.xyz, opacity);
	#endif
}