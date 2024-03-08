#include "common.h"
#include <glad/glad.h>
#include <stdio.h>
#include <sys/stat.h>
#include <glm/glm.hpp>
using namespace glm;
#include <btBulletDynamicsCommon.h>
template <typename T> using vector = btAlignedObjectArray<T>;
static vector<float> &operator<<(vector<float> &a, vec4 b) { return a << b.x, b.y, b.z, b.w; }
static vector<float> &operator,(vector<float> &a, vec4 b) { return a << b; }
static vector<float> &operator<<(vector<float> &a, mat4 b)
{
    return a << b[0], b[1], b[2], b[3];
}

mat3 rotationAlign( vec3 d, vec3 z );
vec3 solve( vec3 p, float r1, float r2, vec3 dir );

typedef struct { int index1, index2; btVector3 halfExtend; }BodyPartData;
extern const BodyPartData bodyPartData[17];
extern const vec3 restPose[22];
extern const ivec2 skeletonMap[21];
int  CreateRagdoll(btDynamicsWorld *);
void ClearBodies(btDynamicsWorld *);

template <typename T> using myList = btAlignedObjectArray<T>;
typedef struct { myList<float> viewBuffer, meshBuffer, textBuffer; }Varying;

#include <ofbx.h>
using namespace ofbx;

Matrix makeIdentity()
{
    dmat4 a(1);
    return (Matrix&)a;
}

IScene *loadFbx(const char *filename);
void EvalPose( vector<vec3> & channels, float t, const IScene *scene,
                  const Object *node, Matrix parentWorld = makeIdentity());

extern "C" Varying mainAnimation(btDynamicsWorld *dynamicsWorld,
            ivec2 iResolution, float iTime, float iTimeDelta, float iFrame, ivec4 iMouse)
{
    static GLuint frame = 0;
    if (frame++ == 0)
    {
        ClearBodies(dynamicsWorld);

        { // add a plane
            dynamicsWorld->setGravity(btVector3(0,-10,0));
            btRigidBody *ground = new btRigidBody( 0, NULL,
                new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
            ground->setDamping(.5, .5);
            ground->setFriction(.5);
            ground->setRestitution(.5);
            dynamicsWorld->addRigidBody(ground);
        }

        {// apply force to ragdoll
            const int colliderID = CreateRagdoll(dynamicsWorld);
            btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[3])
                 ->applyCentralImpulse(btVector3(0,0,-10));
        }
    }

    myList<float> viewBuffer;
    myList<float> meshBuffer;
    myList<float> textBuffer;

    float ti = iTime;
    vec3 ta = vec3(0,1,0);
    vec3 ro = ta + vec3(sin(ti),.3,cos(ti)) * 2.f;
    viewBuffer << ta.x, ta.y, ta.z, 0, ro.x, ro.y, ro.z, 0;

    dynamicsWorld->stepSimulation(iTimeDelta);
    myDebugDraw *dd = dynamic_cast<myDebugDraw*>(dynamicsWorld->getDebugDrawer());
    dd->clearLines();
    dynamicsWorld->debugDrawWorld();
    // dd->drawArc(btVector3(0,1,0), btVector3(0,1,0), btVector3(0,0,1), .5, .5, 0, M_PI*2, btVector3(1,1,0), false);

    static float fps = 0;
    if ((frame & 0xf) == 0) fps = 1. / iTimeDelta;
    char text[32];
    sprintf(text, "%.2f\t\t%.1f fps\t\t%dx%d", iTime, fps, iResolution.x, iResolution.y);

    btVector4 dest = { iResolution.x * 0.5 - iResolution.y * 0.5,
        0, iResolution.y, iResolution.y, };
    btVector4 source = { 0,0,512,512 };
    dd->drawRectangle(dest, source);
    dd->draw2dText(.1 * iResolution.x, .9 * iResolution.y, text, 20);

    // keyframe animation
    const int nJoints = sizeof restPose / sizeof *restPose;
    vec3 local[nJoints];
    local[0] = restPose[0];
    for (ivec2 e : skeletonMap)
    {
        int p = e[0], c = e[1];
        local[c] = restPose[c] - restPose[p];
    }

    enum {
        HIPS = 0,
        NECK = 3,
        HEAD = 4,
        FOOT_L = 8,
        HAND_L = 12,
        FOOT_R = 16,
        HAND_R = 20,
    };

    vec3 jointPos[nJoints] = {};

    vector<vec3> channels;
    channels.resize(11);
    static const IScene *fbxScene = loadFbx("../Data/Standard Walk.fbx");
    EvalPose(channels, iTime, fbxScene, fbxScene->getRoot());

    vec3 local2 = channels[2] * length(local[HEAD]) + channels[1];
    dd->drawSphere((btVector3&)local2, .02, {});
    dd->drawSphere((btVector3&)(channels[1]), .02, {});

    if (1)
    {
        jointPos[HIPS] = channels[0] * length(local[HIPS]);
        jointPos[NECK] = channels[1];
        jointPos[HEAD] = channels[2] * length(local[HEAD]) + channels[1];
        jointPos[FOOT_L] = channels[3];
        jointPos[HAND_L] = channels[4];
        jointPos[FOOT_R] = channels[5];
        jointPos[HAND_R] = channels[6];
    }
    else
    {
        for (int i=0; i<nJoints; i++)
            jointPos[i] = restPose[i];
    }

    // if (0)
    { // solve
        { // position constaints 1
            jointPos[HEAD] = jointPos[NECK] + local[HEAD];
            jointPos[HEAD+1] = jointPos[HEAD] + local[HEAD+1];
        }
        { // 3 bone ik
            jointPos[NECK-2] = jointPos[HIPS] + local[NECK-2];
            // jointPos[NECK-1] = jointPos[NECK] - local[NECK-1];

            {
                int handle = NECK;
                vec3 o = jointPos[handle-2];
                vec3 p = jointPos[handle];
                float r1 = length(local[handle-1]);
                float r2 = length(local[handle]);
                vec3 x = o + solve(p-o, r1, r2, vec3(1,0,0));
                jointPos[NECK-1] = x;
            }
            {
                int handle = NECK-1;
                vec3 o = jointPos[handle-2];
                vec3 p = jointPos[handle];
                float r1 = length(local[handle-1]);
                float r2 = length(local[handle]);
                vec3 x = o + solve(p-o, r1, r2, vec3(1,0,0));
                // jointPos[NECK-2] = x;
            }

            // jointPos[NECK-2] = x2;
        }
        { // position constraints 2
            jointPos[FOOT_R-2] = local[FOOT_R-2] + jointPos[HIPS];
            jointPos[FOOT_L-2] = local[FOOT_L-2] + jointPos[HIPS];
            jointPos[HAND_R-2] = local[HAND_R-2] + jointPos[NECK-1];
            jointPos[HAND_L-2] = local[HAND_L-2] + jointPos[NECK-1];
        }
        // limbs
        vec3 dir[] = { {1,0,0},{-1,0,0},{1,0,0},{-1,0,0} };
        int i = 0;
        for (int handle : { 8,12,16,20 })
        {
            vec3 o = jointPos[handle-2];
            vec3 p = jointPos[handle];
            float r1 = length(local[handle-1]);
            float r2 = length(local[handle]);
            vec3 x = o + solve(p - o, r1, r2, dir[i++]);
            jointPos[handle-1] = x;

            mat3 swi = rotationAlign( (p-x)/r2, local[handle]/r2 );
            jointPos[handle+1] = p + local[handle+1] * swi;
        }
    }

    meshBuffer.clear();
    { // draw plane
        mat4 m = mat4(1);
        m[0][3] = 0.05;
        m[3][3] = intBitsToFloat(-1);
        meshBuffer << m;
    }
    for (auto data : bodyPartData)
    { // draw character colliders
        int p = data.index1;
        int c = data.index2;
        float r = length(local[c]);
        vec3 d = jointPos[c]-jointPos[p];
        mat3 swi = rotationAlign(d/r, local[c]/r);

        mat4 m = transpose(swi);
        vec3 ce = (jointPos[p] + jointPos[c]) * .5f;

        m[3][0] = ce.x;
        m[3][1] = ce.y;
        m[3][2] = ce.z;
        m[0][3] = data.halfExtend.x();
        m[1][3] = data.halfExtend.y();
        m[2][3] = data.halfExtend.z();
        m[3][3] = intBitsToFloat(0);
        meshBuffer << m;
    }

    return { viewBuffer, meshBuffer, textBuffer };
}
