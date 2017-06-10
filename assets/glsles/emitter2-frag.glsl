#version 100
#if GL_ES
#define fragColor gl_FragColor
#define FRAG_COLOR_OUTPUT_DECL
#else
#define FRAG_COLOR_OUTPUT_DECL out vec4 fragColor
#define varying in
#endif

precision highp float;
// Varying (inputs)
varying highp vec3      v_pColorOffset;
// Uniforms
uniform highp float     u_Time;
uniform highp vec3      u_eColor;
// Outputs
FRAG_COLOR_OUTPUT_DECL;

void main()
{
	// Color
    highp vec4 color = vec4(1.0);
    color.rgb = u_eColor + v_pColorOffset;
    color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
    
    // Required OpenGL ES 2.0 outputs
    fragColor = color;
}
