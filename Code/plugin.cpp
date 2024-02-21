#include <btBulletDynamicsCommon.h>
#include <glad/glad.h>
#include <stdio.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using namespace glm;
using boost::container::vector;
#include "bvh.h"

template <class T> vector<T> &operator<<(vector<T> &a, T const& b)
{
    a.push_back(b);
    return a;
}

typedef struct{ vec4 par; mat3x4 pose; }Instance;

extern "C" int mainAnimation(float t)
{
    static GLuint ubo1, ssbo1, frame = 0;
    if (frame++ == 0)
    {
        glGenBuffers(1, &ubo1);
        glGenBuffers(1, &ssbo1);

        vector<vec3> V;

        FILE* file = fopen( "../unity.tri", "r" );
        if (file)
        {
            float a, b, c, d, e, f, g, h, i;
            for (;;)
            {
                int eof = fscanf( file, "%f %f %f %f %f %f %f %f %f\n",
                    &a, &b, &c, &d, &e, &f, &g, &h, &i );
                if (eof < 0) break;
                V << vec3(a, b, c);
                V << vec3(d, e, f);
                V << vec3(g, h, i);
            }
            fclose( file );
        }

        Bvh bvh; bvh.Build(V);
        vector<Tri> const& U = bvh.tri;
        vector<Node> const& N = bvh.bvhNode;

        GLuint ssbo2, ssbo3;
        glGenBuffers(1, &ssbo2);
        glGenBuffers(1, &ssbo3);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo2);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof U[0] * U.size(), U.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo2);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo3);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof N[0] * N.size(), N.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo3);
    }

    btTransform xform;
    xform.setIdentity();
    vector<Instance> I;
    I << Instance{ vec4(vec3(.2), intBitsToFloat(-1)), mat3(1) };
    for (int i=0; i<7; i++)
    {
        mat4 inv;
        xform.setOrigin(btVector3(i-3,0,0));
        xform.setRotation(btQuaternion(btVector3(1,0,0), t));
        xform.getOpenGLMatrix(&inv[0][0]);
        I << Instance{ vec4(vec3(.2), intBitsToFloat(i%4)), mat3x4(transpose(inv)) };
    }

    uint data[] = { (uint)I.size() };
    glBindBuffer(GL_UNIFORM_BUFFER, ubo1);
    glBufferData(GL_UNIFORM_BUFFER, sizeof data, data, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo1);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof I[0] * I.size(), I.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo1);

    return 0;
}
