#if GL_ES
#define fragColor gl_FragColor
#define FRAG_COLOR_OUTPUT_DECL
#else
#define FRAG_COLOR_OUTPUT_DECL out vec4 fragColor;
#define varying in
#endif

precision highp float;

uniform sampler2D diffuse;
uniform sampler2D alpha_only;
uniform float alpha_multiplier;
uniform vec3 overlay_color;
uniform float overlay_color_ratio;
varying vec3 color;
varying vec2 uv;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
	vec4 t = texture2D(diffuse, uv) + vec4(color, 0.0);
	float a = texture2D(alpha_only, uv).r;
	fragColor = (1.0 - overlay_color_ratio) * t + overlay_color_ratio * vec4(overlay_color, t.a);
	fragColor.a *= alpha_multiplier * a;
}
