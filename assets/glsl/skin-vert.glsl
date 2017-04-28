#version 150

precision mediump float;

uniform mat4 MVP;
uniform vec2 vUvOffset;
uniform vec2 vUvScale;
uniform mat4 bone[40];

in vec3 vPos;
in vec3 vCol;
in vec2 vUv;
in vec4 vBw;
in vec4 vBm;
out vec3 color;
out vec2 uv;

void main()
{
	vec4 p = vec4(vPos, 1.0);
	uvec4 bm = uvec4(vBm);
	
	float weight_sum = vBw.x + vBw.y + vBw.z + vBw.w;
	
	vec4 ps = (1.0 - weight_sum) * p
			+ vBw.x * bone[bm.x] * p
			+ vBw.y * bone[bm.y] * p
			+ vBw.z * bone[bm.z] * p
			+ vBw.w * bone[bm.w] * p;
	
    gl_Position = MVP * ps;
	
    color = vCol;
    uv = vUvOffset + vUvScale * vUv;
}
