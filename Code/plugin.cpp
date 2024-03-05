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

static unsigned int Keywords[] = {
    btHashString("mixamorig:Hips").getHash(),
    btHashString("mixamorig:Neck").getHash(),
    btHashString("mixamorig:Head").getHash(),
    btHashString("mixamorig:RightHand").getHash(),
    btHashString("mixamorig:RightFoot").getHash(),
    btHashString("mixamorig:LeftHand").getHash(),
    btHashString("mixamorig:LeftFoot").getHash(),
};

void drawPose(btIDebugDraw *dd, float t, const AnimationLayer *layer,
                  const Object *node, Matrix parentWorld = makeIdentity())
{
    t = mod(t, 1.2f);
    Vec3 translation = node->getLocalTranslation();
    Vec3 rotation = node->getLocalRotation();
    const AnimationCurveNode *channel1 = layer->getCurveNode(*node, "Lcl Translation");
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

    vector<unsigned int> keywords;
    keywords.initializeFromBuffer(Keywords, 11, 11);
    keywords.quickSort(vector<unsigned int>::less());
    int index = keywords.findBinarySearch(btHashString(node->name).getHash());
    if (index != keywords.size())
    {
        btVector3 pos = btVector3( world.m[12], world.m[13], world.m[14] ) * .01;
        const btVector3 halfExtend = btVector3(1,1,1) * 0.05;
        dd->drawAabb(pos-halfExtend, pos+halfExtend, {});
    }

    for (int i=0; node->resolveObjectLink(i); i++)
    {
        const Object *child = node->resolveObjectLink(i);
        if (child->isNode()) drawPose(dd, t, layer, child, world);
    }
}

void drawText( vector<float> & buffer, const vector<int> & map, const char *text)
{
    const int len = strlen(text);
    float xpos = 0., ypos = .0;
    const vec2 size = 24.f / (vec2(16,9)*50.f*512.f/8.0f);

    for (int i=0; i<len; i++)
    {
        const char id = text[i];
        int x = map[id*7+0];
        int y = map[id*7+1];
        int w = map[id*7+2];
        int h = map[id*7+3];
        int xoff = map[id*7+4];
        int yoff = map[id*7+5];
        int xadv = map[id*7+6];
        if (id != ' ')
        {
            buffer << xpos+xoff*size.x, ypos+yoff*size.y, w*size.x, h*size.y,
                float(x), float(y), float(w), float(h);
        }
        xpos += xadv*size.x*.75;
    }
}

typedef struct { int index1, index2; btVector3 halfExtend; }BodyPartData;
extern const BodyPartData bodyPartData[17];
extern const vec3 restPose[22];
extern const ivec2 skeletonMap[21];

int  Physics_CreateRagdoll(btDynamicsWorld *);
void Physics_ClearBodies(btDynamicsWorld *);

extern "C" void mainAnimation(vector<mat4> & I, vector<float> & drawtext,
                              btDynamicsWorld *dynamicsWorld, float iTime, int iFrame)
{
    static vector<int> fontData;
    static const IScene *fbxScene = NULL;
    static GLuint frame = 0;
    if (frame++ == 0)
    {
        { // load font file
            int mn = 0xff, mx = 0;
            fontData.resize(128*7, 0);
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

                    fontData[a*7+0] = b;
                    fontData[a*7+1] = c;
                    fontData[a*7+2] = d;
                    fontData[a*7+3] = e;
                    fontData[a*7+4] = f;
                    fontData[a*7+5] = g;
                    fontData[a*7+6] = h;
                    mn = min(mn, a);
                    mx = max(mx, a);
                }
                fclose(file);
                printf("INFO: loaded file %s\n", filename);
            }
        }

        { // load fbx
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

        Physics_ClearBodies(dynamicsWorld);

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
            const int colliderID = Physics_CreateRagdoll(dynamicsWorld);
            btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[3])
                 ->applyCentralImpulse(btVector3(0,0,-10));
        }
    }

    drawtext.clear();
    drawText(drawtext, fontData, "Hello, World");

    static float lastFrameTime = 0;
    float dt = iTime - lastFrameTime;
    lastFrameTime = iTime;
    dynamicsWorld->stepSimulation(dt);
    btIDebugDraw *dd = dynamicsWorld->getDebugDrawer();
    dd->clearLines();
    dynamicsWorld->debugDrawWorld();
    // dd->drawArc(btVector3(0,1,0), btVector3(0,1,0), btVector3(0,0,1), .5, .5, 0, M_PI*2, btVector3(1,1,0), false);
    drawPose(dd, iTime, fbxScene->getAnimationStack(0)->getLayer(0), fbxScene->getRoot());

    // keyframe animation
    const int nJoints = sizeof restPose / sizeof *restPose;
    vec3 local[nJoints];
    for (ivec2 e : skeletonMap)
    {
        int p = e[0], c = e[1];
        local[c] = restPose[c] - restPose[p];
    }

    enum {
        HIPS = 0,
        NECK = 3,
        HEAD = 4,
        FOOT_R = 8,
        HAND_R = 12,
        FOOT_L = 16,
        HAND_L = 20,
    };

    vec3 jointPos[nJoints] = {};

    { // channels
        float t = iTime*1.5;
        jointPos[NECK] = restPose[NECK] - vec3(0,.05*(sin(t)*.5+.5),0);
        vec3 h = jointPos[HIPS] = restPose[HIPS];// - vec3(0,.1*(sin(t)*.5+.5),0);
        jointPos[FOOT_R] = {  0.1,0.2,0 };
        jointPos[FOOT_L] = { -0.1,0.2,0 };
        jointPos[HAND_R] = {  0.3,1.2,0 };
        jointPos[HAND_L] = { -0.3,1.2,0 };
    }
    { // solve
        { // position constaints 1
            jointPos[HIPS+1] = local[HIPS+1] + jointPos[HIPS];
            jointPos[HEAD] = jointPos[NECK] + local[HEAD];
            jointPos[HEAD+1] = jointPos[HEAD] + local[HEAD+1];
        }
        { // spine
            vec3 o = jointPos[NECK-2];
            vec3 p = jointPos[NECK];
            float r1 = length(local[NECK-1]);
            float r2 = length(local[NECK]);
            vec3 x = o + solve(p-o, r1, r2, vec3(1,0,0));
            jointPos[NECK-1] = x;
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

    if (0) for (vec3 p : jointPos)
    {
        dd->drawSphere(btVector3(p.x, p.y, p.z), .05f, {});
    }

    I.clear();
    { // draw plane
        mat4 m = mat4(1);
        m[0][3] = 0.05;
        m[3][3] = intBitsToFloat(-1);
        I.push_back(m);
    }
    for (auto data : bodyPartData)
    { // draw character colliders

        int p = data.index1;
        int c = data.index2;
        float r = length(local[c]);
        vec3 d = jointPos[c]-jointPos[p];
        mat3 swi = rotationAlign(d/r, local[c]/r);

        mat4 m = transpose(swi);
        vec3 ce = (jointPos[data.index2] + jointPos[data.index1]) * .5f;

        m[3][0] = ce.x;
        m[3][1] = ce.y;
        m[3][2] = ce.z;
        m[0][3] = data.halfExtend.x();
        m[1][3] = data.halfExtend.y();
        m[2][3] = data.halfExtend.z();
        m[3][3] = intBitsToFloat(0);
        I.push_back(m);
    }
}
