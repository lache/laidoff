#version 100

precision mediump float;

uniform sampler2D diffuse;
uniform sampler2D alpha_only;
uniform float alpha_multiplier;
uniform vec3 overlay_color;
uniform float overlay_color_ratio;
varying vec3 color;
varying vec2 uv;
void main()
{
	vec4 t = texture2D(diffuse, uv) + vec4(color, 0.0);
	float a = texture2D(alpha_only, uv).r;
	gl_FragColor = (1.0 - overlay_color_ratio) * t + overlay_color_ratio * vec4(overlay_color, t.a);
	gl_FragColor.a *= alpha_multiplier * a;
}
