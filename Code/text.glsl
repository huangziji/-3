#version 320 es
precision mediump float;

#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

_varying vec2 UV;
layout (location = 0) uniform vec2 iResolution;
layout (binding  = 0) uniform sampler2D iChannel0;

#ifdef _VS
layout (location = 1) in vec4 aDst;
layout (location = 2) in vec4 aSrc;
void main()
{
    vec2 texSize = vec2(textureSize(iChannel0, 0));
    float ar = iResolution.y / iResolution.x;

    UV = vec2(gl_VertexID&1, gl_VertexID/2).yx;
    vec2 vertex = aDst.xy + aDst.zw * UV * vec2(ar, 1);
    UV = aSrc.xy + aSrc.zw * UV;
    UV /= texSize;

    // vflip
    gl_Position = vec4(vertex.x * 2. - 1., 1. - vertex.y * 2., 0, 1);
}
#else
out vec4 fragColor;
void main()
{
    float d = textureLod(iChannel0, UV, 1.).g;
    float t = smoothstep(.4, .5, d);
    fragColor = vec4(vec3(1), t);
}
#endif
