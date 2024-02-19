#include <glad/glad.h>
#include <stdio.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using namespace glm;
using boost::container::vector;
#include "bvh.h"
typedef vector<vec3> Soup;
template <class T> vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; }

extern "C" void mainAnimation(Soup & V)
{
    static AabbTree aabbTree;
    static int frame = 0;
    if (!frame++)
    {
        FILE* file = fopen( "../unity.tri", "r" );
        if (file)
        {
            float a, b, c, d, e, f, g, h, i;
            for (;;)
            {
                int eof = fscanf( file, "%f %f %f %f %f %f %f %f %f\n",
                    &a, &b, &c, &d, &e, &f, &g, &h, &i );
                if (eof < 0) break;
                V << vec3( a, b, c );
                V << vec3( d, e, f );
                V << vec3( g, h, i );
            }
            fclose( file );
        }

        aabbTree.BuildBvh(V);

        vector<vec4> U;
        vector<Node> const& N = aabbTree.bvhNode;
        printf("V : %ld\n", V.size());
        for (auto & t : aabbTree.tri)
        {
            U << vec4(t.vertex0, 1);
            U << vec4(t.vertex1, 1);
            U << vec4(t.vertex2, 1);
        }

        GLuint ssbo1, ssbo2;
        glGenBuffers(1, &ssbo1);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof U[0] * U.size(), U.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo1);
        glGenBuffers(1, &ssbo2);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo2);
        glBufferData(GL_SHADER_STORAGE_BUFFER, N.size() * sizeof N[0], N.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo2);
    }
}
