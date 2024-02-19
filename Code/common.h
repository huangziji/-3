#ifndef COMMON_H
#define COMMON_H
#include <boost/container/vector.hpp>
using boost::container::vector;
#include <glm/glm.hpp>
using namespace glm;

typedef vector<vec3> Soup;

bool reloadShader1(long *, uint, const char *);
bool reloadShader2(long *, uint, const char *);
void *loadPlugin(const char *, const char *);
bool recordVideo(float sec);

#endif // COMMON_H
