#ifndef COMMON_H
#define COMMON_H

bool reloadShader1(long *, unsigned int, const char *);

bool reloadShader2(long *, unsigned int, const char *);

void *loadPlugin(const char *, const char *);

bool recordVideo(float sec);

float spline( const float *k, int n, float t );

#endif // COMMON_H
