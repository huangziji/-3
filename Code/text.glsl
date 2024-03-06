#version 300 es
precision mediump float;

#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

_varying vec2 UV;
uniform sampler2D iChannel0;

#ifdef _VS
layout (location = 1) in vec4 aPosition;
layout (location = 2) in vec4 aTexCoord;
void main()
{
    UV = vec2(gl_VertexID&1, gl_VertexID/2).yx;
    vec2 vertex = aPosition.xy + aPosition.zw * UV;
    UV = aTexCoord.xy + aTexCoord.zw * UV;

    // screen space to ndc
    gl_Position = vec4(vertex.x * 2. - 1., 1. - vertex.y * 2., 0, 1);
}
#else
out vec4 fragColor;
void main()
{
    vec2 texSize = vec2(textureSize(iChannel0, 0));
    float d = textureLod(iChannel0, UV / texSize, 1.).g;
    float t = smoothstep(.4, .5, d);
    fragColor = vec4(vec3(1), t);
}
#endif
