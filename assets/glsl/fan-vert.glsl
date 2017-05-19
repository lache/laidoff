#version 150
precision mediump float;
uniform mat4 MVP;
uniform float rscale[1 + 72 + 1];
uniform float thetascale;
in vec3 vPos;

void main()
{
	// vPos: R, Theta, Index
	
	uvec3 vPosU = uvec3(vPos);
	
	vec2 p;
	p.x = vPos.x * cos(vPos.y * thetascale) * rscale[vPosU.z];
	p.y = vPos.x * sin(vPos.y * thetascale) * rscale[vPosU.z];
	
    gl_Position = MVP * vec4(p, 0.0, 1.0);
}
