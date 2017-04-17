#version 150
#define GLYPH_OUTLINE_MIX_THICKNESS 0.05

precision mediump float;
uniform sampler2D diffuse;
uniform float alpha_multiplier;
uniform vec3 overlay_color;
uniform float overlay_color_ratio;
uniform vec4 glyph_color;
uniform vec4 outline_color;
in vec3 color;
in vec2 uv;
out vec4 fragColor;

void main()
{
    float t = texture(diffuse, uv).r;
    float glyph_alpha = smoothstep(0.5 - GLYPH_OUTLINE_MIX_THICKNESS, 1.0, t);
    float outline_and_glyph_alpha = smoothstep(0.0, 0.5 + GLYPH_OUTLINE_MIX_THICKNESS, t);
    float outline_alpha = outline_and_glyph_alpha - glyph_alpha;
    fragColor = glyph_alpha * glyph_color + outline_alpha * outline_color;
}
