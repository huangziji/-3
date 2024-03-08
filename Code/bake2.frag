#version 320 es
precision mediump float;

float sdRoundedBox( in vec2 p, in vec2 b, in vec4 r )
{
    r.xy = (p.x>0.0)?r.xy : r.zw;
    r.x  = (p.y>0.0)?r.x  : r.y;
    vec2 q = abs(p)-b+r.x;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r.x;
}

float sdPentagon( in vec2 p, in float r )
{
    const vec3 k = vec3(0.809016994,0.587785252,0.726542528);
    p.x = abs(p.x);
    p -= 2.0*min(dot(vec2(-k.x,k.y),p),0.0)*vec2(-k.x,k.y);
    p -= 2.0*min(dot(vec2( k.x,k.y),p),0.0)*vec2( k.x,k.y);
    p -= vec2(clamp(p.x,-r*k.z,r*k.z),r);
    return length(p)*sign(p.y);
}

vec2 getGV(int id)
{
    return vec2(id&15, id/16);
}

layout (location = 0) uniform vec2 iResolution;
layout (location = 0) out vec4 fragColor;
void main(void)
{
    vec2 UV = gl_FragCoord.xy / iResolution.xy;
    UV *= 16.;
    // UV += getGV(2);

    ivec2 id2 = ivec2(floor(UV));
    int id = id2.x + id2.y * 16;

    float d = 0.;
    vec2 p = UV - getGV(id);
    switch (id)
    {
    case 0:
        d = length(p-.5)-.4;
        break;
    case 1:
        d = sdRoundedBox(p-.5, vec2(.45), vec4(.2));
        break;
    case 2:
        d = sdPentagon(p-.5, .3);
        break;
    }

    fragColor.r = smoothstep(.01, .0, d);
}
