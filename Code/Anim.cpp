#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
using namespace glm;

extern const vec3 restPose[] = {
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
    {0.40, 1.55, 0.00},
    {0.65, 1.55, 0.00},
    {0.70, 1.55, 0.00},
    {-.10, 1.00, 0.00},
    {-.10, 0.50, 0.00},
    {-.10, 0.00, 0.00},
    {-.10, 0.00, 0.15},
    {-.15, 1.55, 0.00},
    {-.40, 1.55, 0.00},
    {-.65, 1.55, 0.00},
    {-.70, 1.55, 0.00},
};

extern const ivec2 skeletonMap[] = {
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
extern const BodyPartData bodyPartData[] = {
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

extern const ivec2 constraintData[] = {
    { 0, 1},
    { 1, 2},
    { 2, 3},
    { 3, 4},
    { 0, 5},
    { 5, 6},
    { 6, 7},
    { 2, 8},
    { 8, 9},
    { 9,10},
    { 0,11},
    {11,12},
    {12,13},
    { 2,14},
    {14,15},
    {15,16},
};

void ClearBodies(btDynamicsWorld *physics)
{
    for (int i = physics->getNumConstraints() - 1; i >= 0; i--)
    {
        btTypedConstraint *joint = physics->getConstraint(i);
        physics->removeConstraint(joint);
        delete joint;
    }
    for (int i = physics->getNumCollisionObjects() - 1; i >= 0; i--)
    {
        btCollisionObject *obj = physics->getCollisionObjectArray()[i];
        btRigidBody *body = btRigidBody::upcast(obj);
        if (body)
        {
            if (body->getMotionState())
                delete body->getMotionState();
            if (body->getCollisionShape())
                delete body->getCollisionShape();
        }
        physics->removeCollisionObject(obj);
        delete obj;
    }
}

int CreateRagdoll(btDynamicsWorld *dynamicsWorld)
{
    // create colliders for character
    int colliderId = dynamicsWorld->getNumCollisionObjects();
    for (auto p : bodyPartData)
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
        // rb->forceActivationState(DISABLE_DEACTIVATION);
        // rb->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
        rb->setDamping(.5, .5);
        rb->setFriction(.5);
        rb->setRestitution(.5);
    }
    for (ivec2 ij : constraintData)
    {
        int i = ij.x + colliderId;
        int j = ij.y + colliderId;
        int k = bodyPartData[ij.y].index1;
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
        joint->setLimit(0, 0, 0);
        joint->setLimit(1, 0, 0);
        joint->setLimit(2, 0, 0);
        joint->setLimit(3, -M_PI_2, 0);
        joint->setLimit(4, 0, M_PI * 2);
        joint->setLimit(5, 0, M_PI * 2);
    }
    return colliderId;
}
