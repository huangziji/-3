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

const vec3 restPose[] = {
    {0.00, 1.00, 0.00},
    {0.00, 1.20, 0.00},
    {0.00, 1.40, 0.00},
    {0.00, 1.60, 0.00},
    {0.00, 1.65, 0.00},
    {0.00, 1.85, 0.00},
    {0.10, 1.00, 0.00},
    {0.10, 0.50, 0.00},
    {0.10, 0.00, 0.00},
    {0.10, 0.00, 0.15},
    {0.15, 1.55, 0.00},
    {0.38, 1.55, 0.00},
    {0.60, 1.55, 0.00},
    {0.70, 1.55, 0.00},
    {-.10, 1.00, 0.00},
    {-.10, 0.50, 0.00},
    {-.10, 0.00, 0.00},
    {-.10, 0.00, 0.15},
    {-.15, 1.55, 0.00},
    {-.38, 1.55, 0.00},
    {-.60, 1.55, 0.00},
    {-.70, 1.55, 0.00},
};

static const ivec2 skeletonMap[] = {
    { 0, 1},
    { 1, 2},
    { 2, 3},
    { 3, 4},
    { 4, 5},
    { 0, 6},
    { 6, 7},
    { 7, 8},
    { 8, 9},
    { 2,10},
    {10,11},
    {11,12},
    {12,13},
    { 0,14},
    {14,15},
    {15,16},
    {16,17},
    { 2,18},
    {18,19},
    {19,20},
    {20,21},
};

typedef struct { int index1, index2; btVector3 halfExtend; }BodyPartData;
static const BodyPartData bodypartMap[] = {
     0, 1, {0.15, 0.10, 0.10},
     1, 2, {0.11, 0.10, 0.08},
     2, 3, {0.15, 0.10, 0.10},
     3, 4, {0.04, 0.02, 0.04},
     4, 5, {0.08, 0.10, 0.10},
     6, 7, {0.05, 0.24, 0.05},
     7, 8, {0.05, 0.24, 0.05},
     8, 9, {0.05, 0.05, 0.08},
    10,11, {0.12, 0.06, 0.06},
    11,12, {0.10, 0.06, 0.06},
    12,13, {0.06, 0.06, 0.06},
    14,15, {0.05, 0.24, 0.05},
    15,16, {0.05, 0.24, 0.05},
    16,17, {0.05, 0.05, 0.08},
    18,19, {0.12, 0.06, 0.06},
    19,20, {0.10, 0.06, 0.06},
    20,21, {0.06, 0.06, 0.06},
};

static const ivec2 jointMap[] = {
    {0,1},{1,2},{2,3},{3,4},
    {0,5},{5,6},{6,7},
    {2,8},{8,9},{9,10},
    {0,11},{11,12},{12,13},
    {2,14},{14,15},{15,16},
};

mat3 rotationAlign(vec3, vec3);

mat3 rotateY(float a)
{
    float c=cos(a), s=sin(a);
    return mat3( c, 0, s,
                 0, 1, 0,
                -s, 0, c);
}

extern "C" void mainAnimation(vector<mat4> & I, btDynamicsWorld *dynamicsWorld, float t)
{
    static GLuint frame = 0;
    if (frame++ == 0)
    {
        { // clear dynamics world
            for (int i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
            {
                btTypedConstraint *joint = dynamicsWorld->getConstraint(i);
                dynamicsWorld->removeConstraint(joint);
                delete joint;
            }
            for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btCollisionObject *obj = dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody *body = btRigidBody::upcast(obj);
                if (body)
                {
                    if (body->getMotionState())
                        delete body->getMotionState();
                    if (body->getCollisionShape())
                        delete body->getCollisionShape();
                }
                dynamicsWorld->removeCollisionObject(obj);
                delete obj;
            }
        }

        { // add a plane
            dynamicsWorld->setGravity(btVector3(0,-10,0));
            btRigidBody *ground = new btRigidBody( 0, NULL,
                new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
            ground->setDamping(.5, .5);
            ground->setFriction(.5);
            ground->setRestitution(.5);
            dynamicsWorld->addRigidBody(ground);
        }

        // create colliders for character
        int colliderId = dynamicsWorld->getNumCollisionObjects();
        for (auto p : bodypartMap)
        {
            int i1 = p.index1, i2 = p.index2;
            btVector3 h = p.halfExtend;
            btVector3 a = btVector3(restPose[i1][0], restPose[i1][1], restPose[i1][2]);
            btVector3 b = btVector3(restPose[i2][0], restPose[i2][1], restPose[i2][2]);
            btVector3 c = (a + b) * .5;

            btTransform pose;
            pose.setIdentity();
            pose.setOrigin(c);
            btTransform dummy;
            dummy.setIdentity();
            btVector3 inertia;
            btScalar  mass = h.x()*h.y()*h.z() * 300.;
            btCollisionShape *shape = new btBoxShape(h);
            shape->calculateLocalInertia(mass, inertia);
            btRigidBody *rb = new btRigidBody(
                btRigidBody::btRigidBodyConstructionInfo(
                mass, new btDefaultMotionState( pose, dummy ), shape, inertia) );
            dynamicsWorld->addRigidBody(rb);
            rb->setDamping(.5, .5);
            rb->setFriction(.5);
            rb->setRestitution(.5);
            rb->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
        }
        for (ivec2 ij : jointMap)
        {
            int i = ij.x + colliderId;
            int j = ij.y + colliderId;
            int k = bodypartMap[ij.y].index1;
            btVector3 a = btVector3(restPose[k][0], restPose[k][1], restPose[k][2]);
            btRigidBody *rb1 = btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[i]);
            btRigidBody *rb2 = btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[j]);
            assert(rb1 && rb2);

            btTransform localA, localB;
            localA.setIdentity();
            localB.setIdentity();
            localA.setOrigin(a - rb1->getWorldTransform().getOrigin());
            localB.setOrigin(a - rb2->getWorldTransform().getOrigin());
            btGeneric6DofConstraint *joint = new btGeneric6DofConstraint(
                        *rb1, *rb2, localA, localB, false);
            dynamicsWorld->addConstraint(joint, true);
            joint->setBreakingImpulseThreshold(0xffffffff);
            // joint->setLimit(0, 0, 0);
            // joint->setLimit(1, 0, 0);
            // joint->setLimit(2, 0, 0);
            joint->setLimit(3, -M_PI_2, 0);
            joint->setLimit(4, 0, M_PI * 2);
            joint->setLimit(5, 0, M_PI * 2);
        }

        // apply force to skeleton
        btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[3])
            ->applyCentralImpulse(btVector3(0,0,-10));
    }

    static float lastFrameTime = 0;
    float dt = t - lastFrameTime;
    lastFrameTime = t;
    dynamicsWorld->stepSimulation(dt);
    btIDebugDraw *dd = dynamicsWorld->getDebugDrawer();
    dd->clearLines();
    dynamicsWorld->debugDrawWorld();

    // keyframe animation
    const int nJoints = sizeof restPose / sizeof *restPose;
    vector<vec3> jointPos;
    vector<mat3> jointRot;
    jointPos.resize(nJoints, vec3(0));
    jointRot.resize(nJoints, mat3(1));

    jointRot[1] = rotateY(t);
    jointPos[0] = restPose[0];
    for (auto e : skeletonMap)
    {
        int p = e[0], i = e[1];
        jointPos[i] = restPose[i] - restPose[p];
    }

    for (auto e : skeletonMap)
    {
        int p = e[0], i = e[1];
        jointPos[i] = jointPos[p] + jointPos[i] * jointRot[p];
        jointRot[i] *= jointRot[p];
    }

    I.clear();

    { // draw plane
        mat4 m = mat4(1);
        m[0][3] = -((btStaticPlaneShape*)dynamicsWorld->getCollisionObjectArray()[0]
                ->getCollisionShape())->getPlaneConstant();
        m[3][3] = intBitsToFloat(-1);
        I.push_back(m);
    }
    for (auto data : bodypartMap)
    { // draw character
        mat4 m = transpose(jointRot[data.index1]);
        vec3 c = (jointPos[data.index2] + jointPos[data.index1]) * .5f;

        m[3][0] = c.x;
        m[3][1] = c.y;
        m[3][2] = c.z;
        m[0][3] = data.halfExtend.x();
        m[1][3] = data.halfExtend.y();
        m[2][3] = data.halfExtend.z();
        m[3][3] = intBitsToFloat(0);
        I.push_back(m);
    }
}
