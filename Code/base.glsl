#version 300 es
precision mediump float;

mat3 setCamera(in vec3 ro, in vec3 ta, float cr)
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = cross(cu, cw);
    return mat3(cu, cv, cw);
}

mat4 projectionMatrix(float ar)
{
    float fov = 1.2;
    float n = 0.1, f = 1000.0;
    float p1 = (f+n)/(f-n);
    float p2 = -2.0*f*n/(f-n);
    return mat4( fov/ar, 0,0,0,0, fov, 0,0,0,0, p1,1,0,0,p2,0 );
}

#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

_varying vec3 Pos;

#ifdef _VS
layout (location = 0) in vec4 aVertex;
uniform vec2 iResolution;
uniform float iTime;
void main()
{
    vec3 ta = vec3(0,0,0);
    vec3 ro = ta + vec3(cos(iTime),0.5,sin(iTime)) * 2.5;
    mat3 ca = setCamera(ro, ta, 0.0);
    Pos = aVertex.xyz;

    float ar = iResolution.x/iResolution.y;
    gl_Position = projectionMatrix(ar) * vec4((aVertex.xyz-ro) * ca, 1);
}
#else
layout (location = 0) out vec4 outNormal;
void main()
{
    vec3 nor = normalize(cross(dFdx(Pos), dFdy(Pos)));
    outNormal = vec4(nor, 1);
}
#endif
