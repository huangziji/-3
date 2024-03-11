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

static float length(Vec3 a)
{
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
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

vec3 GetRootMotionAverageVelocity(const IScene *scene)
{
    const AnimationLayer *layer = scene->getAnimationStack(0)->getLayer(0);
    for (int i=0; layer->getCurveNode(i); i++)
    {
        const AnimationCurveNode *channel = layer->getCurveNode(i);
        if (channel->name[0] == 'T')
        {
            vec3 translation = vec3(0);
            const int N = channel->getCurve(0)->getKeyCount() - 1;
            for (int i=0; i<N; i++)
            {
                float x0 = channel->getCurve(0)->getKeyValue()[i+0];
                float y0 = channel->getCurve(1)->getKeyValue()[i+0];
                float z0 = channel->getCurve(2)->getKeyValue()[i+0];
                float x1 = channel->getCurve(0)->getKeyValue()[i+1];
                float y1 = channel->getCurve(1)->getKeyValue()[i+1];
                float z1 = channel->getCurve(2)->getKeyValue()[i+1];
                translation.x += x1-x0;
                translation.y += y1-y0;
                translation.z += z1-z0;
            }
            translation /= N;
            return vec3(translation.x, translation.y, translation.z);
        }
    }

    return vec3(0);
}

float GetAnimationDuration(const IScene *scene)
{
    const AnimationLayer *layer = scene->getAnimationStack(0)->getLayer(0);
    for (int i=0; layer->getCurveNode(i); i++)
    {
        const AnimationCurveNode *channel = layer->getCurveNode(i);
        if (channel)
        {
            int n = channel->getCurve(0)->getKeyCount();
            float f = channel->getCurve(0)->getKeyTime()[n - 1];
            return fbxTimeToSeconds(f);
        }
    }

    return 0;
}

void EvalPoseRecursive( vector<vec3> & out, vector<float> & outLengths, vector<vec3> & axes,
               float t, const IScene *scene, const Object *node,
               float len = 0, Matrix parentWorld = makeIdentity())
{
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
        vec3 q = vec3( parentWorld.m[12], parentWorld.m[13], parentWorld.m[14] );
        axes[index] = out[index] - q;
    }

    for (int i=0; node->resolveObjectLink(i); i++)
    {
        const Object *child = node->resolveObjectLink(i);
        if (child->isNode()) EvalPoseRecursive(out, outLengths, axes, t, scene, child, len, world);
    }
}

void EvalPose(vector<vec3> &out, vector<float> &outLengths, vector<vec3> &axes, float t, const IScene *scene)
{
    EvalPoseRecursive(out, outLengths, axes, t, scene, scene->getRoot());
}
