#version 150
precision mediump float;
// Uniforms
uniform mat4 uProjectionMatrix;
uniform float uK;
uniform float uTime;
// Attributes
in float aTheta;
in vec3 aShade;
// Outputs
out vec3 vShade;

void main()
{
	float x = uTime * cos(uK*aTheta) * sin(aTheta);
	float y = uTime * cos(uK*aTheta) * cos(aTheta);
	
	gl_Position = uProjectionMatrix * vec4(x, y, 0.0, 1.0);
	gl_PointSize = 16.0;
	
	vShade = aShade;
}
