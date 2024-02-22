#include <glad/glad.h>
#include <stdio.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using namespace glm;
using boost::container::vector;
#include "bvh.h"
#include <btBulletDynamicsCommon.h>

template <class T> vector<T> &operator<<(vector<T> &a, T const& b)
{
    a.push_back(b);
    return a;
}

typedef struct{ vec4 par; mat3x4 pose; }Instance;

extern "C" void mainAnimation(float t, btDynamicsWorld *dynamicWorld)
{
    static GLuint ubo1, ssbo1, frame = 0;
    if (frame++ == 0)
    {
        dynamicWorld->setGravity(btVector3(0,-10,0));

        btTransform world, local;
        world.setIdentity();
        local.setIdentity();

        btRigidBody *ground = new btRigidBody( 0, NULL,
            new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
        ground->setDamping(.1, .1);
        ground->setFriction(.5);
        ground->setRestitution(.5);
        dynamicWorld->addRigidBody(ground);

        world.setRotation(btQuaternion(btVector3(0,1,0), t));
        btCollisionShape *shape = new btBoxShape(btVector3(.3,.3,.3));
        float mass = 1.;
        btVector3 inertia = btVector3(0,0,0);
        shape->calculateLocalInertia(mass, inertia);
        world.setOrigin(btVector3(1,5,0));
        btRigidBody *rb1 = new btRigidBody(
            btRigidBody::btRigidBodyConstructionInfo(
            mass, new btDefaultMotionState( world, local ), shape, inertia) );
        rb1->setDamping(.2, .2);
        rb1->setFriction(.5);
        rb1->setRestitution(.5);
        dynamicWorld->addRigidBody(rb1);

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
        glGenBuffers(1, &ubo1);
        glGenBuffers(1, &ssbo1);
        glGenBuffers(1, &ssbo2);
        glGenBuffers(1, &ssbo3);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo2);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof U[0] * U.size(), U.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo2);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo3);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof N[0] * N.size(), N.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo3);
    }

    static float lastFrameTime = 0;
    float dt = t - lastFrameTime;
    lastFrameTime = t;
    dynamicWorld->stepSimulation(dt);
    dynamicWorld->debugDrawWorld();

    vector<Instance> I;
    btCollisionObjectArray const& arr = dynamicWorld->getCollisionObjectArray();
    for (int i=0; i<arr.size(); i++)
    {
        btRigidBody *body = btRigidBody::upcast(arr[i]);
        btCollisionShape *shape = body->getCollisionShape();
        float d1 = -((btStaticPlaneShape*)shape)->getPlaneConstant();
        btVector3 h1 = ((btBoxShape*)shape)->getHalfExtentsWithMargin();
        float r1 = ((btSphereShape*)shape)->getRadius();
        float r2 = ((btCapsuleShape*)shape)->getRadius();

        mat4 mat;
        Instance insta;
        btTransform pose = body->getWorldTransform();
        pose.getOpenGLMatrix(&mat[0][0]);
        insta.pose = mat3x4(transpose(mat));

        switch (shape->getShapeType())
        {
        case STATIC_PLANE_PROXYTYPE:
            insta.par = vec4(d1,0,0,intBitsToFloat(-1));
            break;
        case BOX_SHAPE_PROXYTYPE:
            insta.par = vec4(h1.x(),h1.y(),h1.z(),intBitsToFloat(0));
            break;
        case SPHERE_SHAPE_PROXYTYPE:
            insta.par = vec4(r1,0,0,intBitsToFloat(1));
            break;
        case CAPSULE_SHAPE_PROXYTYPE:
        {
            float h2 = ((btCapsuleShape*)shape)->getHalfHeight();
            insta.par = vec4(h2,r2,0,intBitsToFloat(2));
            break;
        }
        case CYLINDER_SHAPE_PROXYTYPE:
            float h3 = ((btCylinderShape*)shape)->getHalfExtentsWithMargin().y();
            float r3 = ((btCylinderShape*)shape)->getRadius();
            insta.par = vec4(h3,r3,0,intBitsToFloat(3));
            break;
        }

        I.push_back(insta);
    }

    uint data[] = { (uint)I.size() };
    glBindBuffer(GL_UNIFORM_BUFFER, ubo1);
    glBufferData(GL_UNIFORM_BUFFER, sizeof data, data, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo1);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof I[0] * I.size(), I.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo1);
}
