#version 300 es
precision mediump float;

float saturate(float x)
{
    return clamp(x,0.,1.);
}

mat3 setCamera(in vec3 ro, in vec3 ta, float cr)
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = cross(cu, cw);
    return mat3(cu, cv, cw);
}

float map(vec3 pos)
{
    float d = pos.y + .5;
    float d2 = length(pos) - .5;
    d = min(d, d2);
    return d;
}

vec3 calcNormal(vec3 pos)
{
    vec2 e = vec2(1.0,-1.0)*0.5773*0.001;
    return normalize(e.xyy*map( pos + e.xyy ) +
                     e.yyx*map( pos + e.yyx ) +
                     e.yxy*map( pos + e.yxy ) +
                     e.xxx*map( pos + e.xxx ) );
}

float castRay(vec3 ro, vec3 rd)
{
    float t = 0.;
    for (int i=0; i<100; i++)
    {
        float h = map(ro + rd * t);
        t += h;
        if (h < 0.0001 || t > 100.) break;
    }
    return t;
}

uniform vec2 iResolution;
uniform float iTime;
out vec4 fragColor;
void main()
{
    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy) / iResolution.y;
    vec3 ta = vec3(0,0,0);
    vec3 ro = vec3(cos(iTime),0.5,sin(iTime)) * 2.5;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, 1.2));

    float t = castRay(ro, rd);

    vec3 col = vec3(0);
    if (t < 100.)
    {
        const vec3 sun_dir = normalize(vec3(1,2,3));
        vec3 pos = ro + rd * t;
        vec3 nor = calcNormal(pos);
        float sha = step(1., castRay(pos + nor*0.001, sun_dir));

        float sun_dif = saturate(dot(nor, sun_dir))*.9+.1;
        float sky_dif = saturate(dot(nor, vec3(0,1,0)))*.15;
        col += vec3(0.9,0.9,0.5)*sun_dif*sha;
        col += vec3(0.5,0.6,0.9)*sky_dif;
    }
    else
    {
        col += vec3(0.5,0.6,0.9)*1.2 - rd.y*.4;
    }

    col = pow(col, vec3(0.4545));
    fragColor = vec4(col, 1);
}
