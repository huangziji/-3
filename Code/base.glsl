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
layout (location = 0) in vec4 aVertex;
uniform mat2x3 iCamera;
uniform vec2 iResolution;
void main()
{
    vec3 ta = iCamera[0];
    vec3 ro = iCamera[1];
    mat3 ca = setCamera(ro, ta, 0.0);

    float ar = iResolution.x/iResolution.y;
    gl_Position = projectionMatrix(ar) * vec4((aVertex.xyz-ro) * ca, 1);
}
#else
layout (location = 0) out vec4 fragColor;
void main()
{
    fragColor = vec4(1);
}
#endif
