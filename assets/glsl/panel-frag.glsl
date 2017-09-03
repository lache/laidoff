#if GL_ES
#define fragColor gl_FragColor
#define FRAG_COLOR_OUTPUT_DECL
precision mediump float;
#else
#define FRAG_COLOR_OUTPUT_DECL out vec4 fragColor;
#define varying in
#endif

uniform float time;
uniform vec2 resolution;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main() {
	vec2 pos = gl_FragCoord.xy / resolution.xy;
	float inten = sin(time * 1.5) * 0.03 + 0.3;
	vec3 color = vec3(0.0);
	
	color += (1.0 - distance(pos, vec2(1.0))) * vec3(1.0 - pos, 1.0) * 2.8 * inten;
	color += (1.0 - distance(pos, vec2(0.0))) * vec3(pos, 1.0) * 2.8 * inten;
	
	fragColor = vec4(color, 1.0);
	//fragColor = vec4(1.0,0.0,0.0,1.0);
}
