#define GLYPH_OUTLINE_MIX_THICKNESS 0.05

precision mediump float;
uniform sampler2D diffuse;
uniform float alpha_multiplier;
uniform vec3 overlay_color;
uniform float overlay_color_ratio;
uniform vec4 glyph_color;
uniform vec4 outline_color;
varying vec3 color;
varying vec2 uv;

void main()
{
    float t = texture2D(diffuse, uv).r;
    float glyph_alpha = smoothstep(0.5 - GLYPH_OUTLINE_MIX_THICKNESS, 1.0, t);
    float outline_and_glyph_alpha = smoothstep(0.0, 0.5 + GLYPH_OUTLINE_MIX_THICKNESS, t);
    float outline_alpha = outline_and_glyph_alpha - glyph_alpha;
    gl_FragColor = glyph_alpha * glyph_color + outline_alpha * outline_color;
}
