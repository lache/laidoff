#version 150
precision highp float;
// Uniforms
uniform vec3 uColor;
uniform sampler2D uTexture;
// Inputs from vertex shader
in vec3 vShade;
// Outputs
out vec4 fragColor;

void main()
{
	vec4 texture = texture2D(uTexture, gl_PointCoord);
	vec4 color = vec4(uColor + vShade, 1.0);
	color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
	fragColor = texture * color;
}
