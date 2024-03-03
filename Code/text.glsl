#version 300 es
precision mediump float;

#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

_varying vec2 UV;

#ifdef _VS
void main()
{
    UV = vec2(gl_VertexID&1, gl_VertexID/2);
    gl_Position = vec4(UV * 2. - 1., 0, 1);
    UV.y = 1. - UV.y;
}
#else
out vec4 fragColor;
uniform sampler2D iChannel0;
void main()
{
    float d = texture(iChannel0, UV).g;
    float t = smoothstep(.5, .55, d);
    fragColor = vec4(vec3(1), t);
}
#endif
