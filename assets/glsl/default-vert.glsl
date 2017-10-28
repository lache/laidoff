#if GL_ES
#else
#define attribute in
#define varying out
#endif
// mediump causes z-fighting on iOS devices. highp required.
precision mediump float;

uniform mat4 MVP;
uniform vec2 vUvOffset;
uniform vec2 vUvScale;
uniform vec2 vS9Offset;
uniform mat4 M;
attribute vec3 vPos;
attribute vec3 vCol;
attribute vec2 vUv;
attribute vec2 vS9;
varying vec3 color;
varying vec2 uv;
varying vec3 v;

void main()
{
    vec3 p = vPos;
    p.xy += vS9 * vS9Offset;
	vec4 pos = MVP * vec4(p, 1.0);
	v = vec3(M * vec4(p, 1.0));
    gl_Position = pos;
    color = vCol;
    uv = vUvOffset + vUvScale * vUv;
}
