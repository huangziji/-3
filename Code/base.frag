#version 320 es
precision mediump float;

float saturate(float a)
{
    return clamp(a,0.,1.);
}

mat3 setCamera(in vec3 ro, in vec3 ta, float cr)
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = cross(cu, cw);
    return mat3(cu, cv, cw);
}

// plane degined by p (p.xyz must be normalized)
float plaIntersect( in vec3 ro, in vec3 rd, in vec4 p )
{
    float t = -(dot(ro,p.xyz)+p.w)/dot(rd,p.xyz);
    return t > 0. ? t : 1e30f;
}

vec2 castRay(vec3 ro, vec3 rd);

uniform sampler2D iChannel1, iChannel2;
uniform vec2 iResolution;
uniform float iTime;
out vec4 fragColor;
void main()
{
    vec2 screenUV = gl_FragCoord.xy/iResolution.xy;
    vec4 gb = texture2D(iChannel2, screenUV);

    float ti = iTime;
    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy) / iResolution.y;
    vec3 ta = vec3(-1,0,0);
    vec3 ro = ta + vec3(cos(ti),0.5,sin(ti)) * 2.5;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, 1.2));

    vec3 nor = gb.xyz;
    vec2 res = castRay(ro, rd);
    float t = res.x;
    float id = 0.;
    float t2 = plaIntersect(ro, rd, vec4(0,1,0,1.2));
    if (t2 < t)
    {
        t = t2;
        id = 1.;
        nor = vec3(0,1,0);
    }

    vec3 col = vec3(0);
    if (0. < t && t < 100.)
    {
        vec3 mate = id > .5 ? vec3(0.5) : vec3(.4,.5,.6);
            mate *= 2.;

        const vec3 sun_dir = normalize(vec3(1,2,3));
        float sha = step(1., castRay(ro + rd*t + nor*0.0001, sun_dir).x) * .8 + .2;

        float sun_dif = saturate(dot(nor, sun_dir))*.9+.1;
        float sky_dif = saturate(dot(nor, vec3(0,1,0)))*.15;
        col += vec3(0.9,0.9,0.5)*sun_dif*sha*mate;
        col += vec3(0.5,0.6,0.9)*sky_dif;
    }
    else
    {
        col += vec3(0.5,0.6,0.9)*1.2 - rd.y*.4;
    }

    // col += pow(res.y*.02, 2.)*vec3(0,1,0);
    col = pow(col, vec3(0.4545));
    fragColor = vec4(col, 1);
}

struct Tri
{
    vec3 vertex0;
    vec3 vertex1;
    vec3 vertex2;
};

struct Ray
{
    vec3 O, D, rD;
    float t;
};

float IntersectAabb( in Ray ray, in vec3 bmin, in vec3 bmax )
{
    float tx1 = (bmin.x-ray.O.x) * ray.rD.x, tx2 = (bmax.x-ray.O.x) * ray.rD.x;
    float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
    float ty1 = (bmin.y-ray.O.y) * ray.rD.y, ty2 = (bmax.y-ray.O.y) * ray.rD.y;
    tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
    float tz1 = (bmin.z-ray.O.z) * ray.rD.z, tz2 = (bmax.z-ray.O.z) * ray.rD.z;
    tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );
    if (tmax >= tmin && tmin < ray.t && tmax > 0.) return tmin; else return 1e30f;
}

void IntersectTri( inout Ray ray, in Tri tri )
{
    vec3 edge1 = tri.vertex1 - tri.vertex0;
    vec3 edge2 = tri.vertex2 - tri.vertex0;
    vec3 h = cross( ray.D, edge2 );
    float a = dot( edge1, h );
    if (a > -0.0001f && a < 0.0001f) return; // ray parallel to triangle
    float f = 1. / a;
    vec3 s = ray.O - tri.vertex0;
    float u = f * dot( s, h );
    if (u < 0. || u > 1.) return;
    vec3 q = cross( s, edge1 );
    float v = f * dot( ray.D, q );
    if (v < 0. || u + v > 1.) return;
    float t = f * dot( edge2, q );
    if (t > 0.0001f) ray.t = min( ray.t, t );
}

struct Node
{
    vec3 aabbMin;
    int leftFirst;
    vec3 aabbMax;
    int primCount;
};

layout (std430, binding = 0) buffer IN_0 {
    Tri data[];
};

layout (std430, binding = 1) buffer IN_1 {
    Node node[];
};

vec2 castRay(vec3 ro, vec3 rd)
{
    float depth = 0.;
    Ray ray = Ray(ro, rd, 1./rd, 1e30f);
    int stack[64];
    int stackPtr = 0;
    int currentNode = 0;
    for (;;)
    {
        ++depth;
        Node n = node[currentNode];
        if (n.primCount > 0)
        {
            for (int i=0; i<n.primCount; i++)
                IntersectTri(ray, data[n.leftFirst + i]);
            if (stackPtr == 0) break; else currentNode = stack[--stackPtr];
        }
        else
        {
            float dist1 = IntersectAabb( ray, n.aabbMin, n.aabbMax );
            float dist2 = IntersectAabb( ray, n.aabbMin, n.aabbMax );
            int child1 = n.leftFirst;
            int child2 = n.leftFirst + 1;
            if (dist1 > dist2)
            {
                float tmp = dist1; dist1 = dist2, dist2 = tmp;
                ++child1, --child2;
            }
            if (dist1 == 1e30f)
            {
                if (stackPtr == 0) break; else currentNode = stack[--stackPtr];
            }
            else
            {
                currentNode = child1;
                if (dist2 < 1e30f) stack[stackPtr++] = child2;
            }
        }
    }
    return vec2(ray.t, depth);
}
