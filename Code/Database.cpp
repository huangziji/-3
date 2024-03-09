#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
using namespace glm;
#include <ofbx.h>
using namespace ofbx;
template <typename T> using vector = btAlignedObjectArray<T>;

static Matrix makeIdentity()
{
    dmat4 a(1);
    return (Matrix&)a;
}

static Matrix operator*(Matrix a, Matrix b)
{
    dmat4 c = (dmat4&)a * (dmat4&)b;
    return (Matrix&)c;
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

static float length(Vec3 a)
{
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}

static Vec3 operator +(Vec3 a, Vec3 b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

void EvalPose( vector<vec3> & out, vector<float> & outLengths, float t, const IScene *scene,
                  const Object *node, float len = 0, Matrix parentWorld = makeIdentity())
{

    t = mod(t, 1.2f);
    Vec3 translation = node->getLocalTranslation();
    Vec3 rotation = node->getLocalRotation();
    const AnimationLayer *layer = scene->getAnimationStack(0)->getLayer(0);
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
    len += length(node->getLocalTranslation());

    static unsigned int data[] = {
        btHashString("mixamorig:Hips").getHash(),
        btHashString("mixamorig:Neck").getHash(),
        btHashString("mixamorig:Head").getHash(),
        btHashString("mixamorig:LeftFoot").getHash(),
        btHashString("mixamorig:LeftHand").getHash(),
        btHashString("mixamorig:RightFoot").getHash(),
        btHashString("mixamorig:RightHand").getHash(),

        btHashString("mixamorig:LeftUpLeg").getHash(),
        btHashString("mixamorig:LeftShoulder").getHash(),
        btHashString("mixamorig:RightUpLeg").getHash(),
        btHashString("mixamorig:RightShoulder").getHash(),
    };

    vector<unsigned int> keywords;
    keywords.initializeFromBuffer(data, 11, 11);
    int index = keywords.findLinearSearch(btHashString(node->name).getHash());
    if (index < keywords.size())
    {
        out[index] = vec3( world.m[12], world.m[13], world.m[14] );
        outLengths[index] = len;
    }

    for (int i=0; node->resolveObjectLink(i); i++)
    {
        const Object *child = node->resolveObjectLink(i);
        if (child->isNode()) EvalPose(out, outLengths, t, scene, child, len, world);
    }
}
