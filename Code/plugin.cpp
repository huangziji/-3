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
static const BodyPartData bodyPartData[] = {
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

mat3 rotationAlign( vec3 d, vec3 z );
vec3 solve( vec3 p, float r1, float r2, vec3 dir );

mat3 rotateY(float a)
{
    float c=cos(a), s=sin(a);
    return mat3( c, 0, s,
                 0, 1, 0,
                -s, 0, c);
}

#include <ofbx.h>
using namespace ofbx;

// const btHashString keywords = "mixamorig:" "Hips" "Spine2" "Head"
    // "RightShoulder" "RightHand" "RightUpLeg" "RightFoot";


Matrix makeIdentity()
{
    dmat4 a(1);
    return (Matrix&)a;
}

Matrix operator*(Matrix a, Matrix b)
{
    dmat4 c = (dmat4&)a * (dmat4&)b;
    return (Matrix&)c;
}

void drawPose(btIDebugDraw *dd, float t, const AnimationLayer *layer,
                  const Object *node, Matrix parentWorld = makeIdentity())
{
    t = mod(t, 1.2f);
    Vec3 translation = node->getLocalTranslation();
    Vec3 rotation = node->getLocalRotation();
    const AnimationCurveNode *channel1 = layer->getCurveNode(*node, "Lcl Translationx");
    const AnimationCurveNode *channel2 = layer->getCurveNode(*node, "Lcl Rotation");
    if (channel1)
    {
        translation = channel1->getNodeLocalTransform(t);
    }
    if (channel2)
    {
        rotation = channel2->getNodeLocalTransform(t);
    }

    Matrix world = parentWorld * node->evalLocal(translation, rotation);
    btVector3 pos = btVector3( world.m[12], world.m[13], world.m[14] ) * .01;
    btVector3 qos = btVector3( parentWorld.m[12], parentWorld.m[13], parentWorld.m[14] ) * .01;
    dd->drawLine(qos, pos, {});
    const btVector3 halfExtend = btVector3(1,1,1) * 0.05;
    dd->drawAabb(pos-halfExtend, pos+halfExtend, {});

    for (int i=0; node->resolveObjectLink(i); i++)
    {
        const Object *child = node->resolveObjectLink(i);
        if (child->isNode()) drawPose(dd, t, layer, child, world);
    }
}

extern "C" void mainAnimation(vector<char> & alloc, vector<mat4> & I,
                              btDynamicsWorld *dynamicsWorld, float iTime, int iFrame)
{
    if (iFrame == 0)
    {
    }

    static const IScene *fbxScene = NULL;
    static GLuint frame = 0;
    if (frame++ == 0)
    {
        { // load font file
            vector<int> fontData;
            const char *filename = "../Data/arial.fnt";
            FILE *file = fopen(filename, "r");
            if (file)
            {
                fscanf(file, "%*[^\n]\n%*[^\n]\n%*[^\n]\n%*[^\n]\n");
                while (1)
                {
                    long cur = ftell(file);
                    int a,b,c,d,e,f,g,h;
                    int eof = fscanf(file, "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d"
                                           "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d",
                        &a,&b,&c,&d,&e,&f,&g,&h);
                    if (eof < 0) break;
                    fseek(file, cur, SEEK_SET);
                    fscanf(file, "%*[^\n]\n");

                    fontData << a,b,c,d,e,f,g,h;
                    // printf("%d\n", eof);
                    // printf("%d %d %d %d %d %d %d %d\n", a,b,c,d,e,f,g,h);
                }
                fclose(file);
                printf("INFO: loaded file %s\n", filename);
            }
        }
        {
            const char *filename = "../Standard Walk.fbx";
            FILE *f = fopen(filename, "rb");
            if (f)
            {
                btClock stop;
                stop.reset();
                fseek(f, 0, SEEK_END);
                long length = ftell(f);
                rewind(f);
                u8 data[length];
                fread(data, 1, length, f);
                fclose(f);
                fbxScene = load(data, length, 0);
                const char *err = getError();
                if (strlen(err)) fprintf(stderr, "ERROR: %s\n", err);
                unsigned long long elapsedTime = stop.getTimeMilliseconds();
                printf("INFO: loaded file %s. It took %ld milliseconds\n", filename, elapsedTime);
            }
        }

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
            rb->setDamping(.5, .5);
            rb->setFriction(.5);
            rb->setRestitution(.5);
        }
        for (ivec2 ij : jointMap)
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
            // joint->setEnabled(false);
        }

        // deactivate ragdoll
        for (int i=colliderId; i<dynamicsWorld->getNumCollisionObjects(); i++)
        {
            btCollisionObject *obj = dynamicsWorld->getCollisionObjectArray()[i];
            obj->forceActivationState(DISABLE_DEACTIVATION);
            obj->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
        }

        // apply force to ragdoll
        btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[3])
            ->applyCentralImpulse(btVector3(0,0,-10));
    }

    static float lastFrameTime = 0;
    float dt = iTime - lastFrameTime;
    lastFrameTime = iTime;
    dynamicsWorld->stepSimulation(dt);
    btIDebugDraw *dd = dynamicsWorld->getDebugDrawer();
    dd->clearLines();
    dynamicsWorld->debugDrawWorld();
    dd->drawArc(btVector3(0,1,0), btVector3(0,1,0), btVector3(0,0,1), .5, .5, 0, M_PI*2, btVector3(1,1,0), false);
    drawPose(dd, iTime, fbxScene->getAnimationStack(0)->getLayer(0), fbxScene->getRoot());

    // keyframe animation
    const int nJoints = sizeof restPose / sizeof *restPose;
    vector<vec3> jointPos;
    vector<mat3> jointRot;
    jointPos.resize(nJoints, vec3(0));
    jointRot.resize(nJoints, mat3(1));

    for (ivec2 e : skeletonMap)
    {
        int p = e[0], c = e[1];
        jointPos[c] = restPose[c] - restPose[p];
    }

    float t = iTime;
    jointPos[0] = restPose[0] - vec3(0,.1*(sin(t)*.5+.5),0);
    for (int i : { 0, 1, 2, 3, 4, 5, 9, 13, 17 })
    {
        int c = skeletonMap[i][1];
        int p = skeletonMap[i][0];
        jointPos[c] = jointPos[p] + jointPos[c] * jointRot[p];
        jointRot[c] *= jointRot[p];
    }

    vec3 target[] = { {.1,.2,0}, {.3,1.2,0}, {-.1,.2,0}, {-.3,1.2,0}, };
    vec3 dir[] = { {1,0,0},{-1,0,0},{1,0,0},{-1,0,0} };
    int jointIdx[] = { 8, 12, 16, 20 };
    for (int i=0; i<4; i++)
    {
        int p = jointIdx[i];
        int x = p - 1, o = p - 2;
        vec3 local1 = jointPos[x];
        vec3 local2 = jointPos[p];
        float r1 = length(local1);
        float r2 = length(local2);
        jointPos[p] = target[i];
        jointPos[x] = jointPos[o] + solve(jointPos[p]-jointPos[o], r1, r2, dir[i]);
        jointRot[o] = rotationAlign( (jointPos[x]-jointPos[o])/r1, (local1)/r1 );
        jointRot[x] = rotationAlign( (jointPos[p]-jointPos[x])/r2, (local2)/r2 );

        jointRot[p] *= jointRot[x];
        jointPos[p+1] = jointPos[p] + jointPos[p+1] * jointRot[p];
    }


    I.clear();
    { // draw plane
        mat4 m = mat4(1);
        m[0][3] = 0.05;
        m[3][3] = intBitsToFloat(-1);
        I.push_back(m);
    }
    for (auto data : bodyPartData)
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
