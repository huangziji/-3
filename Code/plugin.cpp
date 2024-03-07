#include "common.h"
#include <glad/glad.h>
#include <stdio.h>
#include <sys/stat.h>
#include <glm/glm.hpp>
using namespace glm;
#include <btBulletDynamicsCommon.h>
template <typename T> using vector = btAlignedObjectArray<T>;
static vector<float> &operator<<(vector<float> &a, float b)
{
    a.push_back(b);
    return a;
}
static vector<float> &operator,(vector<float> &a, float b) { return a << b; }

mat3 rotationAlign( vec3 d, vec3 z );
vec3 solve( vec3 p, float r1, float r2, vec3 dir );

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

float length(Vec3 a)
{
    return sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
}

static unsigned int Keywords[] = {
    btHashString("mixamorig:Hips").getHash(),
    btHashString("mixamorig:Neck").getHash(),
    btHashString("mixamorig:Head").getHash(),
    btHashString("mixamorig:RightFoot").getHash(),
    btHashString("mixamorig:RightHand").getHash(),
    btHashString("mixamorig:LeftFoot").getHash(),
    btHashString("mixamorig:LeftHand").getHash(),
};

void GetPose( vector<vec3> & channels, float t, const AnimationLayer *layer,
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
    keywords.initializeFromBuffer(Keywords, 7, 7);
    keywords.quickSort(vector<unsigned int>::less());
    int index = keywords.findBinarySearch(btHashString(node->name).getHash());
    if (index < keywords.size())
    {
        float len = 0.;
        len = 100;
        // len += length(node->getLocalTranslation());
        // len += length(node->getParent()->getParent()->getLocalTranslation());
        btVector3 pos = btVector3( world.m[12], world.m[13], world.m[14] ) / len;
        channels[index] = { pos.x(), pos.y(), pos.z() };
    }

    for (int i=0; node->resolveObjectLink(i); i++)
    {
        const Object *child = node->resolveObjectLink(i);
        if (child->isNode()) GetPose(channels, t, layer, child, world);
    }
}

void draw2dText( vector<float> & buffer, const vector<int> & info, const char *text)
{
    const int len = strlen(text);
    float xpos = .1, ypos = .92;
    float lineHei = 20.f / (9*50.f*512.f/8.0f);
    float ar = 9./16.;

    for (int i=0; i<len; i++)
    {
        const char id = text[i];
        int x = info[id*7+0];
        int y = info[id*7+1];
        int w = info[id*7+2];
        int h = info[id*7+3];
        int xoff = info[id*7+4];
        int yoff = info[id*7+5];
        int xadv = info[id*7+6];

        if (!isspace(id))
        {
            buffer << xpos+xoff*lineHei*ar, ypos+yoff*lineHei,
                    w*lineHei*ar, h*lineHei,
                float(x), float(y), float(w), float(h);
            xpos += lineHei * ar * xadv *.75;
        }
        else
        {
            xpos += lineHei * 25 * ((id == '\t') * 4 + 1);
        }
    }
}

vector<int> loadFnt(const char *filename)
{
    vector<int> fontdata;
    fontdata.resize(128*7, 0);
    FILE *file = fopen(filename, "r");
    if (file)
    {
        fscanf(file, "%*[^\n]\n%*[^\n]\n%*[^\n]\n%*[^\n]\n");
        for (;;)
        {
            long cur = ftell(file);
            int a,b,c,d,e,f,g,h;
            int eof = fscanf(file, "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d"
                                   "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d",
                &a,&b,&c,&d,&e,&f,&g,&h);
            if (eof < 0) break;
            fseek(file, cur, SEEK_SET);
            fscanf(file, "%*[^\n]\n");

            fontdata[a*7+0] = b;
            fontdata[a*7+1] = c;
            fontdata[a*7+2] = d;
            fontdata[a*7+3] = e;
            fontdata[a*7+4] = f;
            fontdata[a*7+5] = g;
            fontdata[a*7+6] = h;
        }
        fclose(file);
        printf("INFO: loaded file %s\n", filename);
    }
    return fontdata;
}

IScene *loadFbx(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return NULL;
    }

    btClock stop;
    stop.reset();

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    u8 data[length];
    fread(data, 1, length, f);
    fclose(f);

    IScene *fbxScene = load(data, length, 0);
    const char *err = getError();
    if (strlen(err)) fprintf(stderr, "ERROR: %s\n", err);
    unsigned long long elapsedTime = stop.getTimeMilliseconds();
    printf("INFO: loaded file %s. It took %ld milliseconds\n", filename, elapsedTime);

    return fbxScene;
}

static vector<float> &operator<<(vector<float> &a, mat4 b)
{
    return a <<
        b[0][0], b[0][1], b[0][2], b[0][3],
        b[1][0], b[1][1], b[1][2], b[1][3],
        b[2][0], b[2][1], b[2][2], b[2][3],
        b[3][0], b[3][1], b[3][2], b[3][3];
}

typedef struct { int index1, index2; btVector3 halfExtend; }BodyPartData;
extern const BodyPartData bodyPartData[17];
extern const vec3 restPose[22];
extern const ivec2 skeletonMap[21];
int  CreateRagdoll(btDynamicsWorld *);
void ClearBodies(btDynamicsWorld *);

template <typename T> using myList = btAlignedObjectArray<T>;
typedef struct { myList<float> viewBuffer, meshBuffer, textBuffer; }Varying;

extern "C" Varying mainAnimation(btDynamicsWorld *dynamicsWorld,
            ivec2 iResolution, float iTime, float iTimeDelta, float iFrame, ivec4 iMouse
                                 )
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

    static const IScene *fbxScene = loadFbx("../Data/Standard Walk.fbx");
    static const vector<int> fontinfo = loadFnt("../Data/arial.fnt");

    dynamicsWorld->stepSimulation(iTimeDelta);
    btIDebugDraw *dd = dynamicsWorld->getDebugDrawer();
    dd->clearLines();
    dynamicsWorld->debugDrawWorld();
    // dd->drawArc(btVector3(0,1,0), btVector3(0,1,0), btVector3(0,0,1), .5, .5, 0, M_PI*2, btVector3(1,1,0), false);

    static float fps = 0;
    if ((frame & 0xf) == 0) fps = 1. / iTimeDelta;
    char text[32];
    sprintf(text, "%.2f\t\t%.1f fps\t\t%d x %d", iTime, fps, iResolution.x, iResolution.y);
    textBuffer.clear();
    draw2dText(textBuffer, fontinfo, text);

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

    vector<vec3> channels;
    channels.resize(7);
    GetPose(channels, iTime, fbxScene->getAnimationStack(0)->getLayer(0), fbxScene->getRoot());
    // if (0)
    {
        jointPos[HIPS] = channels[4];// * length(local[HIPS]);
        jointPos[NECK] = channels[1];// * length(local[NECK]);
        jointPos[HEAD] = channels[3];// * length(local[HEAD]);
        jointPos[FOOT_R] = channels[6];// * length(local[FOOT_R]);
        jointPos[HAND_R] = channels[2];// * length(local[HAND_R]);
        jointPos[FOOT_L] = channels[5];// * length(local[FOOT_L]);
        jointPos[HAND_L] = channels[0];// * length(local[HAND_L]);
    }

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
