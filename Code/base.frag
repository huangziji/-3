#version 320 es
precision mediump float;

// https://iquilezles.org/articles/palettes/
vec3 palette(float t)
{
    float TWOPI = 3.14159 * 2.;
    return 0.5 - 0.5 * cos(TWOPI * (vec3(0.8, 0.9, 0.9) * t + vec3(0.25, .5, 0)));
}

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

struct Hit
{
    float t;
    vec3 nor;
    int id;
};

Hit castRay(in vec3 ro, in vec3 rd);

uniform mat2x3 iCamera;
uniform vec2 iResolution;
out vec4 fragColor;
void main()
{
    vec3 ta = iCamera[0];
    vec3 ro = iCamera[1];

    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy) / iResolution.y;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, 1.2));

    Hit h = castRay(ro, rd);
    float t = h.t;

    vec3 col = vec3(0);
    if (0. < t && t < 100.)
    {
        vec3 nor = h.nor;
        vec3 mate = palette(float(h.id) + 24111.23);

        const vec3 sun_dir = normalize(vec3(1,2,3));
        float t2 = castRay(ro + rd*t + nor*0.0003, sun_dir).t;
        float sha = float(0. > t2 || t2 > 100.) * .8 + .2;

        float sun_dif = saturate(dot(nor, sun_dir))*.8+.2;
        float sky_dif = saturate(dot(nor, vec3(0,1,0)))*.2;
        col += vec3(0.9,0.9,0.5)*sun_dif*sha*mate;
        col += vec3(0.5,0.6,0.9)*sky_dif;
    }
    else
    {
        col += vec3(0.5,0.6,0.9)*1.2 - rd.y*.4;
    }

    col = pow(col, vec3(0.4545));
    fragColor = vec4(col, 1);

    const float n = 0.1, f = 1000.0;
    const float p10 = (f+n)/(f-n), p11 = -2.0*f*n/(f-n); // from perspective matrix
    float ssd = min(t, f) * dot(rd, normalize(ta-ro)); // convert camera dist to screen space dist
    float ndc = p10 + p11/ssd; // inverse of linear depth
    gl_FragDepth = (ndc*gl_DepthRange.diff + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
}

struct Instance{
    vec4 par;
    mat4x3 pose;
};

layout (std140, binding = 0) uniform U0 {
    int instanceCount;
};

layout (std430, binding = 0, row_major) buffer IN_0 { Instance instance[]; };

vec4 plaIntersect( in vec3 ro, in vec3 rd, in vec4 p );
vec4 boxIntersect( in vec3 ro, in vec3 rd, in vec3 h );
vec4 sphIntersect( in vec3 ro, in vec3 rd, float ra );
vec4 capIntersect( in vec3 ro, in vec3 rd, float he, float ra );
vec4 cylIntersect( in vec3 ro, in vec3 rd, float he, float ra );
vec4 conIntersect( in vec3 ro, in vec3 rd, float he, float ra, float rb );
vec4 bvhIntersect(in vec3 ro, in vec3 rd, int nodeIdx);

Hit castRay(in vec3 ro, in vec3 rd)
{
    float t = 1e30f;
    vec3 nor;
    int id;

    for (int i=0; i<instanceCount; i++)
    {
        vec4 h;
        vec3 par = instance[i].par.xyz;
        int type = floatBitsToInt(instance[i].par.w);
        mat3 rot = mat3(instance[i].pose);
        vec3 pos = instance[i].pose[3];
        vec3 O = (ro-pos) * rot;
        vec3 D = rd * rot;
        switch(type)
        {
        case -1:
            h = plaIntersect(O, D, vec4(0,1,0,par.x));
            break;
        case 0:
            h = boxIntersect(O, D, par);
            break;
        case 1:
            h = sphIntersect(O, D, par.x);
            break;
        case 2:
            h = capIntersect(O, D, par.x, par.y);
            break;
        case 3:
            h = cylIntersect(O, D, par.x, par.y);
            break;
        case 4:
            h = conIntersect(O, D, par.x, par.y, 0.);
            break;
        default:
            h = bvhIntersect(ro, rd, 0);
            break;
        }
        if (0. < h.x && h.x < t)
        {
            t = h.x;
            nor = rot * h.yzw;
            id = i;
        }
    }
    return Hit(t, nor, id);
}

vec4 plaIntersect( in vec3 ro, in vec3 rd, in vec4 p )
{
    float t = -(dot(ro,p.xyz)+p.w)/dot(rd,p.xyz);
    return vec4(t, p.xyz);
}
vec4 boxIntersect( in vec3 ro, in vec3 rd, vec3 h )
{
    vec3 m = 1.0/rd;
    vec3 n = m*ro;
    vec3 k = abs(m)*h;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
    if( tN>tF || tF<0.0) return vec4(-1.0);
    vec3 nor = (tN>0.0) ? step(vec3(tN),t1) : step(t2,vec3(tF));
        nor *= -sign(rd);
    return vec4( tN, nor );
}
vec4 sphIntersect( in vec3 ro, in vec3 rd, float ra )
{
    float b = dot( ro, rd );
    vec3 qc = ro - b*rd;
    float h = ra*ra - dot( qc, qc );
    if( h<0.0 ) return vec4(-1.0);
    h = sqrt( h );
    float t = -b-h;
    return vec4(t, (ro + t*rd)/ra);
}
vec4 cylIntersect( in vec3 ro, in vec3 rd, float he, float ra )
{
    float k2 = 1.0        - rd.y*rd.y;
    float k1 = dot(ro,rd) - ro.y*rd.y;
    float k0 = dot(ro,ro) - ro.y*ro.y - ra*ra;

    float h = k1*k1 - k2*k0;
    if( h<0.0 ) return vec4(-1.0);
    h = sqrt(h);
    float t = (-k1-h)/k2;

    // body
    float y = ro.y + t*rd.y;
    if( y>-he && y<he ) return vec4( t, (ro + t*rd - vec3(0.0,y,0.0))/ra );

    // caps
    t = ( ((y<0.0)?-he:he) - ro.y)/rd.y;
    if( abs(k1+k2*t)<h ) return vec4( t, vec3(0.0,sign(y),0.0) );

    return vec4(-1.0);
}
vec4 capIntersect( in vec3 ro, in vec3 rd, float he, float ra )
{
    float k2 = 1.0        - rd.y*rd.y;
    float k1 = dot(ro,rd) - ro.y*rd.y;
    float k0 = dot(ro,ro) - ro.y*ro.y - ra*ra;

    float h = k1*k1 - k2*k0;
    if( h<0.0 ) return vec4(-1.0);
    h = sqrt(h);
    float t = (-k1-h)/k2;

    // body
    float y = ro.y + t*rd.y;
    if( y>-he && y<he ) return vec4( t, (ro + t*rd - vec3(0.0,y,0.0))/ra );

    // caps
    vec3 oc = ro + vec3(0.0,he,0.0) * -sign(y);
    float b = dot(rd,oc);
    float c = dot(oc,oc) - ra*ra;
    h = b*b - c;
    t = -b - sqrt(h);
    if( h>0.0 ) return vec4( t, (oc + t*rd)/ra );

    return vec4(-1.0);
}
float dot2(vec3 a) { return dot(a, a); }
vec4 conIntersect( in vec3 ro, in vec3 rd, float he, float ra, float rb )
{
    ro += vec3(0.0,he,0.0)*0.5;
    vec3  ob = ro - vec3( 0.0,he,0.0);

    //caps
         if( ro.y<0.0 ) { if( dot2(ro*rd.y-rd*ro.y)<(ra*ra*rd.y*rd.y) ) return vec4(-ro.y/rd.y,-vec3(0.0,1.0,0.0)); }
    else if( ro.y>he  ) { if( dot2(ob*rd.y-rd*ob.y)<(rb*rb*rd.y*rd.y) ) return vec4(-ob.y/rd.y, vec3(0.0,1.0,0.0)); }

    // body
    float m4 = dot(rd,ro);
    float m5 = dot(ro,ro);
    float rr = ra - rb;
    float hy = he*he + rr*rr;

    float k2 = he*he    - rd.y*rd.y*hy;
    float k1 = he*he*m4 - ro.y*rd.y*hy + ra*(rr*he*rd.y*1.0 );
    float k0 = he*he*m5 - ro.y*ro.y*hy + ra*(rr*he*ro.y*2.0 - he*he*ra);

    float h = k1*k1 - k2*k0;
    if( h<0.0 ) return vec4(-1.0);

    float t = (-k1-sqrt(h))/k2;

    float y = ro.y + t*rd.y;
    if( y>0.0 && y<he )
    {
        return vec4(t, normalize(
        he*he*(ro+t*rd) + vec3(0.0,rr*he*ra - hy*y,0.0)
        ));
    }

    return vec4(-1.0);
}
vec3 triIntersect( in vec3 ro, in vec3 rd, in vec3 v0, in vec3 v1, in vec3 v2 )
{
    vec3 v1v0 = v1 - v0;
    vec3 v2v0 = v2 - v0;
    vec3 rov0 = ro - v0;
    vec3  n = cross( v1v0, v2v0 );
    vec3  q = cross( rov0, rd );
    float d = 1.0/dot( rd, n );
    float u = d*dot( -q, v2v0 );
    float v = d*dot(  q, v1v0 );
    float t = d*dot( -n, rov0 );
    if( u<0.0 || v<0.0 || (u+v)>1.0 ) t = 1e30f;
    return vec3( t, u, v );
}

//===========================================================================//

struct Data
{
    vec3 min;
    int id;
    vec3 max;
    int count;
};

layout (std430, binding = 1) buffer IN_1 { float aVertex[]; };
layout (std430, binding = 2) buffer IN_2 { Data aNode[]; };

vec3 getVertex3(int idx)
{
    idx *= 3;
    return vec3(aVertex[idx+0], aVertex[idx+1], aVertex[idx+2]);
}

vec4 bvhIntersect(in vec3 ro, in vec3 rd, int nodeIdx)
{
    float t = 1e30f;
    vec3 nor;

    int stack[32];
    int stackPtr = 0; // reset
    while (true)
    {
        Data x = aNode[nodeIdx];
        if (x.count > 0)
        {
            for (int i=0; i<x.count; i++)
            {
                int idx = x.id + i;
                vec3 v0 = getVertex3(idx*3+0);
                vec3 v1 = getVertex3(idx*3+1);
                vec3 v2 = getVertex3(idx*3+2);
                vec3 n1 = normalize(cross(v1-v0,v2-v0));
                vec3 h = triIntersect(ro, rd, v0, v1, v2);
                if (h.x < t) t = h.x, nor = n1;
            }
        }
        else
        {
            vec3 ce = (x.max + x.min) * .5;
            vec3 he = (x.max - x.min) * .5;
            float dist1 = boxIntersect(ro-ce, rd, he).x;
            float dist2 = boxIntersect(ro-ce, rd, he).x;
            int child1 = x.id;
            int child2 = x.id+1;
            if (dist1 > dist2)
            {
                float tmp = dist1; dist1 = dist2, dist2 = tmp;
                int tmp2 = child1; child1 = child2, child2 = tmp2;
            }
            if (dist1 < 1e30f)
            {
                nodeIdx = child1;
                if (dist2 < 1e30f) stack[stackPtr++] = child2;
                continue;
            }
        }

        if (stackPtr == 0) break; else nodeIdx = stack[--stackPtr];
    }
    return vec4(t, nor);
}
