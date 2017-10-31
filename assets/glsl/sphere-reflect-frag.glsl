#ifdef GL_ES
#define fragColor gl_FragColor
#define FRAG_COLOR_OUTPUT_DECL
#define TEX texture2D
#else
#define FRAG_COLOR_OUTPUT_DECL out vec4 fragColor;
#define varying in
#define TEX texture
#endif

precision mediump float;

uniform sampler2D diffuse;
uniform float alpha_multiplier;
uniform vec3 overlay_color;
uniform float overlay_color_ratio;
uniform vec3 sphere_pos[3];
uniform vec3 sphere_col[3];
uniform float sphere_col_ratio[3];
varying vec3 color;
varying vec2 uv;
varying vec3 v;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
    vec4 t = TEX(diffuse, uv); // + vec4(color, 0.0);
    fragColor = (1.0 - overlay_color_ratio) * t + overlay_color_ratio * vec4(overlay_color, t.a);
    fragColor.a *= alpha_multiplier;
	
	float power = 15.0;
	float offset = 0.1;
	float d0 = distance(v, sphere_pos[0]);
	float d1 = distance(v, sphere_pos[1]);
	float d2 = distance(v, sphere_pos[2]);
	
	float w = 0.5;
	
	float r0 = 1.0 / (1.0 + exp(power * (w*d0 - offset)));
	float r1 = 1.0 / (1.0 + exp(power * (w*d1 - offset)));
	float r2 = 1.0 / (1.0 + exp(power * (w*d2 - offset)));
	
	fragColor.rgb += r0 * sphere_col[0] * sphere_col_ratio[0];
	fragColor.rgb += r1 * sphere_col[1] * sphere_col_ratio[1];
	fragColor.rgb += r2 * sphere_col[2] * sphere_col_ratio[2];
}
