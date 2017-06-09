#version 150
precision mediump float;
// Uniforms
uniform mat4 uProjectionMatrix;
uniform float uK;
// Attributes
in float aTheta;

void main()
{
	float x = cos(uK*aTheta) * sin(aTheta);
	float y = cos(uK*aTheta) * cos(aTheta);
	
	gl_Position = uProjectionMatrix * vec4(x, y, 0.0, 1.0);
	gl_PointSize = 16.0;
}
