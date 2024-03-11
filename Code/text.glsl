#version 320 es
precision mediump float;

#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

flat _varying int Id;
_varying vec2 UV;
layout (location = 0) uniform vec2 iResolution;
layout (binding  = 4) uniform sampler2D iChannel[2];

#ifdef _VS
layout (location = 1) in vec4 aDst;
layout (location = 2) in vec4 aSrc;
void main()
{
    Id = int(aSrc.w < 0.);
    UV = vec2(gl_VertexID&1, gl_VertexID/2).yx; // vflip change the winding

    vec2 pos = aDst.xy + aDst.zw * UV;
    UV = aSrc.xy + abs(aSrc.zw) * UV;
    pos /= iResolution;
    // UV /= texSize;

    gl_Position = vec4(pos * 2. - 1., 0, 1);
    gl_Position.y *= -1.; // vflip
}
#else
out vec4 fragColor;
void main()
{
    vec2 invTexSize = 1./vec2(textureSize(iChannel[Id], 0));
    if (Id == 0)
    {
        float d = textureLod(iChannel[Id], UV*invTexSize, 1.).r;
        float t = smoothstep(.4, .5, d);
        fragColor = vec4(vec3(1), t);
    }
    else
    {
        float d = textureLod(iChannel[Id], UV*invTexSize, 0.).r;
        fragColor = vec4(vec3(1), d);
    }
}
#endif
