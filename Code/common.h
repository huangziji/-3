#ifndef COMMON_H
#define COMMON_H
#include <glm/glm.hpp>
using namespace glm;

bool reloadShader1(long *, uint, const char *);

bool reloadShader2(long *, uint, const char *);

void *loadPlugin(const char *, const char *);

bool recordVideo(float sec);

mat3x3 rotationAlign( vec3 d, vec3 z );

vec3 solve( vec3 p, float r1, float r2, vec3 dir );

float spline( const float *k, int n, float t );

#endif // COMMON_H
