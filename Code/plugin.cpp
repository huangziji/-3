#include "ragdoll.cpp"
#include "common.h"
#include <glad/glad.h>
#include <stdio.h>
#include <sys/stat.h>
#include <glm/glm.hpp>
using namespace glm;
#include <btBulletDynamicsCommon.h>
template <typename T> using vector = btAlignedObjectArray<T>;
template <class T> vector<T> &operator,(vector<T> &a, T const& b) { return a << b; }
template <class T> vector<T> &operator<<(vector<T> &a, T const& b)
{
    a.push_back(b);
    return a;
}

struct { float x,y,z,a,b,c; } const RagdollDesc[] = {
    0.00, 1.00, 0.00, 0.15, 0.10, 0.10,
    0.00, 1.20, 0.00, 0.11, 0.10, 0.08,
    0.00, 1.40, 0.00, 0.15, 0.10, 0.10,
    0.00, 1.60, 0.00, 0.04, 0.02, 0.04,
    0.00, 1.65, 0.00, 0.08, 0.10, 0.10,
    0.00, 1.85, 0.00, -1.00, -1.00, -1.00,
    0.10, 1.00, 0.00, 0.05, 0.24, 0.05,
    0.10, 0.50, 0.00, 0.05, 0.24, 0.05,
    0.10, 0.00, 0.00, 0.05, 0.05, 0.08,
    0.10, 0.00, 0.15, -1.00, -1.00, -1.00,
    0.15, 1.55, 0.00, 0.12, 0.06, 0.06,
    0.38, 1.55, 0.00, 0.10, 0.06, 0.06,
    0.60, 1.55, 0.00, 0.06, 0.06, 0.06,
    0.70, 1.55, 0.00, -1.00, -1.00, -1.00,
    -0.10, 1.00, 0.00, 0.05, 0.24, 0.05,
    -0.10, 0.50, 0.00, 0.05, 0.24, 0.05,
    -0.10, 0.00, 0.00, 0.05, 0.05, 0.08,
    -0.10, 0.00, 0.15, -1.00, -1.00, -1.00,
    -0.15, 1.55, 0.00, 0.12, 0.06, 0.06,
    -0.38, 1.55, 0.00, 0.10, 0.06, 0.06,
    -0.60, 1.55, 0.00, 0.06, 0.06, 0.06,
    -0.70, 1.55, 0.00, -1.00, -1.00, -1.00,
};

extern "C" void mainAnimation(vector<mat4> & I, btDynamicsWorld *dynamicWorld, float t)
{
    {
        static long lastModTime;
        const char *filename = "../Code/rig.csv";

        struct stat libStat;
        int err = stat(filename, &libStat);
        if (err == 0 && lastModTime != libStat.st_mtime)
        {
            lastModTime = libStat.st_mtime;
            printf("INFO: reloading file %s\n", filename);
            FILE *file = fopen(filename, "r");
            if (file)
            {
                char buffer[256];
                for (;;)
                {
                    long cur = ftell(file);
                    int a;
                    float b,c,d,e,f,g,h,i;
                    int eof = fscanf( file, "%[^,],%d,%f,%f,%f,%f,%f,%f,%f",
                        buffer, &a, &b, &c, &d, &e, &f, &g, &h, &i );
                    if (eof < 0) break;
                    fseek(file, cur, SEEK_SET);
                    fgets(buffer, sizeof buffer, file);
                    // printf("%d, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f,\n",
                           // a, b, c, d, e, f, g);
                }
                fclose( file );
            }
        }
    }

    static GLuint frame = 0;
    if (frame++ == 0)
    {
        { // clear dynamics world
            for (int i = dynamicWorld->getNumConstraints() - 1; i >= 0; i--)
            {
                btTypedConstraint *joint = dynamicWorld->getConstraint(i);
                dynamicWorld->removeConstraint(joint);
                delete joint;
            }
            for (int i = dynamicWorld->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btCollisionObject *obj = dynamicWorld->getCollisionObjectArray()[i];
                btRigidBody *body = btRigidBody::upcast(obj);
                if (body)
                {
                    if (body->getMotionState())
                        delete body->getMotionState();
                    if (body->getCollisionShape())
                        delete body->getCollisionShape();
                }
                dynamicWorld->removeCollisionObject(obj);
                delete obj;
            }
        }

        dynamicWorld->setGravity(btVector3(0,-10,0));
        btRigidBody *ground = new btRigidBody( 0, NULL,
            new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
        ground->setDamping(.5, .5);
        ground->setFriction(.5);
        ground->setRestitution(.5);
        dynamicWorld->addRigidBody(ground);

        const ivec2 bodypartIdx[] = {
            {0,1},{1,2},{2,3},{3,4},{4,5},
            {6,7},{7,8},{8,9},
            {10,11},{11,12},{12,13},
            {14,15},{15,16},{16,17},
            {18,19},{19,20},{20,21},
        };
        const ivec2 jointIdx[] = {
            {1,0},{1,2},{2,3},{3,4},
            {0,6},{6,7},{7,8},
            {2,10},{10,11},{11,12},
            {0,14},{14,15},{15,16},
            {2,18},{18,19},{19,20},
        };

        vector<btRigidBody*> tmpBodyparts;
        tmpBodyparts.resize(sizeof RagdollDesc/sizeof RagdollDesc[0], NULL);
        for (ivec2 ij : bodypartIdx)
        {
            int i = ij.x, j = ij.y;
            btVector3 e = btVector3(RagdollDesc[i].a, RagdollDesc[i].b, RagdollDesc[i].c);
            btVector3 a = btVector3(RagdollDesc[i].x, RagdollDesc[i].y, RagdollDesc[i].z);
            btVector3 b = btVector3(RagdollDesc[j].x, RagdollDesc[j].y, RagdollDesc[j].z);
            btVector3 c = (a + b) * .5;

            btCollisionShape *shape = new btBoxShape(e);
            btTransform pose;
            pose.setIdentity();
            pose.setOrigin(c);
            btTransform dummy;
            dummy.setIdentity();
            btVector3 inertia;
            btScalar  mass = e.x()*e.y()*e.z() * 300.;
            shape->calculateLocalInertia(mass, inertia);
            btRigidBody *rb = new btRigidBody(
                btRigidBody::btRigidBodyConstructionInfo(
                mass, new btDefaultMotionState( pose, dummy ), shape, inertia) );
            dynamicWorld->addRigidBody(rb);
            tmpBodyparts[i] = rb;
            rb->setDamping(.5,.5);
            rb->setFriction(.5);
            rb->setRestitution(.5);
            // rb->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
        }
        for (ivec2 ij : jointIdx)
        {
            int i = ij.x, j = ij.y;
            btVector3 a = btVector3(RagdollDesc[j].x, RagdollDesc[j].y, RagdollDesc[j].z);
            btRigidBody *rb1 = tmpBodyparts[i];
            btRigidBody *rb2 = tmpBodyparts[j];
            assert(rb1 && rb2);

            btTransform localA, localB;
            localA.setIdentity();
            localB.setIdentity();
            localA.setOrigin(a - rb1->getWorldTransform().getOrigin());
            localB.setOrigin(a - rb2->getWorldTransform().getOrigin());
            btGeneric6DofConstraint *joint = new btGeneric6DofConstraint(
                        *rb1, *rb2, localA, localB, false);
            dynamicWorld->addConstraint(joint, i==2||i==0);
            joint->setBreakingImpulseThreshold(0xffff);
        }

        btRigidBody::upcast(dynamicWorld->getCollisionObjectArray()[3])
                ->applyCentralImpulse(btVector3(0,0,-10));
    }

    static float lastFrameTime = 0;
    float dt = t - lastFrameTime;
    lastFrameTime = t;
    dynamicWorld->stepSimulation(dt);
    btIDebugDraw *dd = dynamicWorld->getDebugDrawer();
    dd->clearLines();
    dynamicWorld->debugDrawWorld();

    I.clear();
    btCollisionObjectArray const& arr = dynamicWorld->getCollisionObjectArray();
    for (int i=0; i<arr.size(); i++)
    {
        btRigidBody *body = btRigidBody::upcast(arr[i]);
        btCollisionShape *shape = body->getCollisionShape();
        float d1 = -((btStaticPlaneShape*)shape)->getPlaneConstant();
        btVector3 h1 = ((btBoxShape*)shape)->getHalfExtentsWithMargin();
        float h2 = ((btCylinderShape*)shape)->getHalfExtentsWithMargin().y();
        float h3 = ((btConeShape*)shape)->getHeight();
        float r1 = ((btSphereShape*)shape)->getRadius();
        float r2 = ((btCapsuleShape*)shape)->getRadius();
        float r4 = ((btConeShape*)shape)->getRadius();

        mat4 m;
        btTransform pose = body->getWorldTransform();
        pose.getOpenGLMatrix(&m[0][0]);

        switch (shape->getShapeType()) {
        case STATIC_PLANE_PROXYTYPE:
            m[0][3] = d1, m[3][3] = intBitsToFloat(-1);
            break;
        case BOX_SHAPE_PROXYTYPE:
            m[0][3] = h1.x(), m[1][3] = h1.y(),
            m[2][3] = h1.z(), m[3][3] = intBitsToFloat(0);
            break;
        case SPHERE_SHAPE_PROXYTYPE:
            m[0][3] = r1, m[3][3] = intBitsToFloat(1);
            break;
        case CAPSULE_SHAPE_PROXYTYPE:
            m[0][3] = ((btCapsuleShape*)shape)->getHalfHeight(),
            m[1][3] = r2,
            m[3][3] = intBitsToFloat(2);
            break;
        case CYLINDER_SHAPE_PROXYTYPE:
            m[0][3] = h2,
            m[1][3] = ((btCylinderShape*)shape)->getRadius(),
            m[3][3] = intBitsToFloat(3);
            break;
        case CONE_SHAPE_PROXYTYPE:
            m[0][3] = h3, m[1][3] = r4,
            m[3][3] = intBitsToFloat(4);
            break;
        }

        I.push_back(m);
    }
}
