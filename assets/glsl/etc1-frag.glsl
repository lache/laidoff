#version 150

precision mediump float;
uniform sampler2D diffuse;
uniform sampler2D alpha_only;
uniform float alpha_multiplier;
uniform vec3 overlay_color;
uniform float overlay_color_ratio;
in vec3 color;
in vec2 uv;
out vec4 fragColor;

void main()
{
	vec4 t = texture(diffuse, uv) + vec4(color, 0.0);
	float a = texture(alpha_only, uv).r;
	fragColor = (1.0 - overlay_color_ratio) * t + overlay_color_ratio * vec4(overlay_color, t.a);
	fragColor.a *= alpha_multiplier * a;
}
