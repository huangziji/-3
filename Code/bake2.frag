#version 320 es
precision mediump float;

float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float sdTriangle( in vec2 p, in vec2 p0, in vec2 p1, in vec2 p2 )
{
    vec2 e0 = p1-p0, e1 = p2-p1, e2 = p0-p2;
    vec2 v0 = p -p0, v1 = p -p1, v2 = p -p2;
    vec2 pq0 = v0 - e0*clamp( dot(v0,e0)/dot(e0,e0), 0.0, 1.0 );
    vec2 pq1 = v1 - e1*clamp( dot(v1,e1)/dot(e1,e1), 0.0, 1.0 );
    vec2 pq2 = v2 - e2*clamp( dot(v2,e2)/dot(e2,e2), 0.0, 1.0 );
    float s = sign( e0.x*e2.y - e0.y*e2.x );
    vec2 d = min(min(vec2(dot(pq0,pq0), s*(v0.x*e0.y-v0.y*e0.x)),
                     vec2(dot(pq1,pq1), s*(v1.x*e1.y-v1.y*e1.x))),
                     vec2(dot(pq2,pq2), s*(v2.x*e2.y-v2.y*e2.x)));
    return -sqrt(d.x)*sign(d.y);
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
    // UV += getGV(4);

    ivec2 id2 = ivec2(floor(UV));
    int id = id2.x + id2.y * 16;

    float d = 1.;
    vec2 p = UV - getGV(id)-.5;
    vec2 q;
    switch (id)
    {
    case 0:
        d = length(p)-.35;
        break;
    case 1:
        d = abs(sdBox(p, vec2(.22)) - .2) - .02;
        break;
    case 2:
        q = p - vec2(.15,0);
        q.x = abs(q.x+.15)-.15;
        d = sdBox(q, vec2(.07,.3)) - .03;
        break;
    case 3:
        q = p - vec2(-.15,0);
        d = sdTriangle(q, vec2(0,-.3), vec2(0,.3), vec2(.4,0)) - .02;
        break;
    case 4:
        q = vec2(.2, 0) - p;
        d = sdTriangle(q, vec2(0,-.3), vec2(0,.3), vec2(.4,0)) - .02;
        float d2 = sdBox(q-vec2(.4,0), vec2(.05,.25)) - .04;
        d = min(d, d2);
        break;
    }

    // float d2 = abs(sdBox(p, vec2(.22)) - .2) - .02;
    // d = min(d, d2);

    fragColor.r = smoothstep(.01, .0, d);
}
