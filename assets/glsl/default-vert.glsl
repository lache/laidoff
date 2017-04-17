#version 150

precision mediump float;

uniform mat4 MVP;
uniform vec2 vUvOffset;
uniform vec2 vUvScale;
uniform vec2 vS9Offset;
in vec3 vPos;
in vec3 vCol;
in vec2 vUv;
in vec2 vS9;
out vec3 color;
out vec2 uv;

void main()
{
	vec3 p = vPos;
	p.xy += vS9 * vS9Offset;
    gl_Position = MVP * vec4(p, 1.0);
    color = vCol;
    uv = vUvOffset + vUvScale * vUv;
}
