#ifndef COMMON_H
#define COMMON_H

bool loadShader1(long *, unsigned int, const char *);

bool loadShader2(long *, unsigned int, const char *);

void *loadPlugin(const char *, const char *);

bool recordVideo(float sec);

float spline( const float *k, int n, float t );

#endif // COMMON_H
