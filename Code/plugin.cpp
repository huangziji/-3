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

#include <ofbx.h>
using namespace ofbx;

IScene *loadFbx(const char *filename);
void EvalPose( vector<vec3> & channels, vector<float> & lens, vector<vec3> & axes, float t, const IScene *scene);
float GetAnimationDuration(const IScene *scene);
vec3 GetRootMotionAverageVelocity(const IScene *scene);

template <typename T> using myList = btAlignedObjectArray<T>;
typedef struct { myList<float> viewBuffer, meshBuffer; }Varying;

extern "C" bool mainAnimation(Varying *out, btDynamicsWorld *dynamicsWorld,
            ivec2 iResolution, float iTime, float iTimeDelta, float iFrame, ivec4 iMouse)
{
    static const IScene *fbxScene = loadFbx("../Data/Standard Walk.fbx");
    static const float fbxDuration = GetAnimationDuration(fbxScene);
    // static const vec3 fbxVel = GetRootMotionAverageVelocity(fbxScene);

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
            CreateRagdoll(dynamicsWorld);
            btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[3])
                 ->applyCentralImpulse(btVector3(0,0,-10));
        }
    }

    const vec3 controlPoints[] = {
        {2,2,2}, {2,2,2}, {2,2,-2}, {-0,3,-5}, {-0,3,-5},
    };

    vector<float> knots;
    knots.resize(15);
    for (int i=0; i<3; i++) for (int j=0; j<5; j++)
    {
        knots[i*5+j] = controlPoints[j][i];
    }

    out->viewBuffer.clear();
    out->meshBuffer.clear();

    float ti = iTime * 1.2;
    vec3 ta = vec3(0,1,0);
    vec3 ro = ta + vec3(sin(ti),.3,cos(ti)) * 2.f;
    out->viewBuffer << ta.x, ta.y, ta.z, 0, ro.x, ro.y, ro.z, 0;

    dynamicsWorld->stepSimulation(iTimeDelta);
    myDebugDraw *dd = dynamic_cast<myDebugDraw*>(dynamicsWorld->getDebugDrawer());
    dd->clearLines();
    dynamicsWorld->debugDrawWorld();
    // dd->drawArc(btVector3(0,1,0), btVector3(0,1,0), btVector3(0,0,1), .5, .5, 0, M_PI*2, btVector3(1,1,0), false);

    for (float t=0.; t<1.; t+=.03)
    for (int i=0; i<5; i++)
    {
        float x = spline(&knots[0], 5, t);
        float y = spline(&knots[5], 5, t);
        float z = spline(&knots[10], 5, t);
        float x2 = spline(&knots[0], 5, t+.03);
        float y2 = spline(&knots[5], 5, t+.03);
        float z2 = spline(&knots[10], 5, t+.03);
        dd->drawLine({x,y,z}, {x2,y2,z2}, {});
    }

    for (int i=0; i<countof(controlPoints); i++)
    {
        vec3 p = controlPoints[i];
        dd->drawSphere({p.x, p.y, p.z}, .02, {});
    }

    static float fps = 0;
    if ((frame & 0xf) == 0) fps = 1. / iTimeDelta;

    // dd->drawRectangle({ iResolution.x * 0.5 - iResolution.y * 0.5,
                       // 0, iResolution.y, iResolution.y, }, { 0,0,iResolution.y,iResolution.y });
    const int size = iResolution.y/16;
    dd->drawRectangle({iResolution.x * .04, iResolution.y * (.9-.01), size,size},
                      { size*4,0, size, size });
    dd->drawRectangle({iResolution.x * .08, iResolution.y * (.9-.01), size,size},
                      { size*2,0, size, size });
    dd->draw2dText(.15 * iResolution.x, .9 * iResolution.y, 20, "%.2f\t\t%.1f fps\t\t%dx%d", iTime, fps, iResolution.x, iResolution.y);
    // dd->drawRectangle({ iResolution.x * .02, iResolution.y * .9, size, size*30 }, { size*5,0, size, size });

    // keyframe animation
    const int nJoints = countof(restPose);
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
    vector<float> lengths;
    vector<vec3> axes;
    channels.resize(11);
    lengths.resize(11);
    axes.resize(11);
    EvalPose(channels, lengths, axes, mod(iTime, fbxDuration), fbxScene);

#if 0
    {
        for (int i=0; i<nJoints; i++)
            jointPos[i] = restPose[i];
    }
#else
    { // solve

        vec3 a = channels[0] / lengths[0];
        a.x = a.z = 0;
        vec3 b = (channels[1]-channels[0]) / (lengths[1]-lengths[0]);
        vec3 c = (channels[2]-channels[1]) / (lengths[2]-lengths[1]);
        vec3 e = (channels[3]-channels[7]) / (lengths[3]-lengths[7]);
        vec3 f = (channels[4]-channels[8]) / (lengths[4]-lengths[8]);
        vec3 g = (channels[5]-channels[9]) / (lengths[5]-lengths[9]);
        vec3 h = (channels[6]-channels[10]) / (lengths[6]-lengths[10]);

        axes[1] = normalize(cross(b, axes[1]));
        axes[3] = normalize(cross(e, axes[3]));
        axes[4] = normalize(cross(f, axes[4]));
        axes[5] = normalize(cross(g, axes[5]));
        axes[6] = normalize(cross(h, axes[6]));

        {
            float lenSpine = length(local[NECK]) + length(local[NECK-1]) + length(local[NECK-2]);
            jointPos[HIPS] = a * length(local[HIPS]);
            jointPos[NECK] = jointPos[HIPS] + b * lenSpine;
            jointPos[HEAD] = jointPos[NECK] + c * length(local[HEAD]);
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
                vec3 x = o + solve(p-o, r1, r2, axes[1]);
                jointPos[NECK-1] = x;
            }
            {
                int handle = NECK-1;
                vec3 o = jointPos[handle-2];
                vec3 p = jointPos[handle];
                float r1 = length(local[handle-1]);
                float r2 = length(local[handle]);
                vec3 x = o + solve(p-o, r1, r2, axes[1]);
                // jointPos[NECK-2] = x;
            }

            // jointPos[NECK-2] = x2;
        }

        { // position constraints 2
            jointPos[FOOT_R-2] = local[FOOT_R-2] + jointPos[HIPS];
            jointPos[FOOT_L-2] = local[FOOT_L-2] + jointPos[HIPS];
            jointPos[HAND_R-2] = local[HAND_R-2] + jointPos[NECK-1];
            jointPos[HAND_L-2] = local[HAND_L-2] + jointPos[NECK-1];
            jointPos[FOOT_L] = jointPos[FOOT_L-2] + e;
            jointPos[HAND_L] = jointPos[HAND_L-2] + f;
            jointPos[FOOT_R] = jointPos[FOOT_R-2] + g;
            jointPos[HAND_R] = jointPos[HAND_R-2] + h;
        }

        // limbs
        vec3 ppp[] = { e,f,g,h };
        int i = 0;
        for (int handle : { 8,12,16,20 })
        {
            float r1 = length(local[handle-1]);
            float r2 = length(local[handle]);

            vec3 o = jointPos[handle-2];
            vec3 p = o + ppp[i] * (r1 + r2);
            jointPos[handle] = p;
            vec3 x = o + solve(p - o, r1, r2, axes[i + 3]);
            i += 1;
            jointPos[handle-1] = x;

            mat3 swi = rotationAlign( (p-x)/r2, local[handle]/r2 );
            jointPos[handle+1] = p + local[handle+1] * swi;
        }
    }
#endif

    for (auto i : { 0, 3, 4, 8, 12, 16, 20, })
    {
        dd->drawSphere((btVector3&)(jointPos[i]), .02, {});
    }

    { // draw plane
        mat4 m = mat4(1);
        m[0][3] = 0.05;
        m[3][3] = intBitsToFloat(-1);
        out->meshBuffer << m;
    }
    for (auto data : bodyPartData)
    { // draw character colliders
        int p = data.index1;
        int c = data.index2;
        float r = length(local[c]);
        vec3 d = jointPos[c]-jointPos[p];
        mat3 swi = transpose(rotationAlign(d/r, local[c]/r));

        mat4 m = swi;
        vec3 ce = (jointPos[p] + jointPos[c]) * .5f;

        m[3][0] = ce.x;
        m[3][1] = ce.y;
        m[3][2] = ce.z;
        m[0][3] = data.halfExtend.x();
        m[1][3] = data.halfExtend.y();
        m[2][3] = data.halfExtend.z();
        m[3][3] = intBitsToFloat(0);
        out->meshBuffer << m;
    }

    return frame == 0;
}
