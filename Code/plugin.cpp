#include "common.h"
#include <glad/glad.h>
#include <stdio.h>
#include <boost/container/vector.hpp>
using boost::container::vector;
#include <btBulletDynamicsCommon.h>

template <class T> vector<T> &operator,(vector<T> &a, T const& b) { return a << b; }
template <class T> vector<T> &operator<<(vector<T> &a, T const& b)
{
    a.push_back(b);
    return a;
}

typedef struct{ vec4 par; mat3x4 pose; }Instance;
#include <sys/stat.h>

extern "C" void mainAnimation(btAlignedObjectArray<Instance> & I, btDynamicsWorld *dynamicWorld, float t)
{
    static vector<int> parents;
    static vector<btVector3> bindPos;
    static vector<int> shapeType;
    static vector<btVector3> shapeDesc;
    bool dirty = false;

    {
        static long lastModTime;
        const char *filename = "../Code/rig.csv";

        struct stat libStat;
        int err = stat(filename, &libStat);
        if (err == 0 && lastModTime != libStat.st_mtime)
        {
            parents.clear();
            bindPos.clear();
            shapeDesc.clear();
            shapeType.clear();
            parents << -1;
            bindPos << btVector3();
            shapeType << -1;
            shapeDesc << btVector3();
            dirty = true;

            lastModTime = libStat.st_mtime;
            printf("INFO: reloading file %s\n", filename);
            FILE *file = fopen(filename, "r");
            if (file)
            {
                char buffer[256];
                for (;;)
                {
                    long cur = ftell(file);
                    int b,c;
                    float d,e,f,g,h,i;
                    int eof = fscanf( file, "%[^,],%d,%d,%f,%f,%f,%f,%f,%f",
                        buffer, &b, &c, &d, &e, &f, &g, &h, &i );
                    if (eof < 0) break;
                    fseek(file, cur, SEEK_SET);
                    fgets(buffer, sizeof buffer, file);
                    // printf("%d %f %f %f %f %f %f\n",
                           // p, a, b, c, d, e, f);

                    parents << b;
                    shapeType << c;
                    bindPos << btVector3(d,e,f);
                    shapeDesc << btVector3(g,h,i);
                }
                fclose( file );
            }
        }
    }

    static GLuint frame = 0;
    if (frame++ == 0 || dirty)
    {
#if 1
        // clear scene
        btCollisionObjectArray const& objs = dynamicWorld->getCollisionObjectArray();
        for (int i = objs.size() - 1; i >= 0; i--)
        {
            btCollisionObject *obj = objs[i];
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
#endif

        dynamicWorld->setGravity(btVector3(0,-10,0));
        btRigidBody *ground = new btRigidBody( 0, NULL,
            new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
        ground->setDamping(.5, .5);
        ground->setFriction(.5);
        ground->setRestitution(.5);
        dynamicWorld->addRigidBody(ground);

        for (int i=0; i<bindPos.size(); i++)
        {
            if (shapeType[i] <= 0) continue;

            btVector3 desc = shapeDesc[i];
            btVector3 a = bindPos[i];
            btVector3 b = bindPos[parents[i]];
            btVector3 c = (a + b) * 0.5f;
            btScalar x = desc.x();
            btScalar y = desc.y() > 0 ? desc.y() : (a - b).length() - x*2;
            btScalar z = desc.z();
            btScalar t = shapeType[i];
            rotationAlign(vec3(1), vec3(1));

            btTransform world, local;
            world.setIdentity();
            local.setIdentity();
            world.setOrigin( c );
            world.getBasis().setEulerYPR(M_PI_2*(t == 2),M_PI_2*(t == 3),0);

            // btCollisionShape *shape = new btBoxShape(btVector3(x,y,z));
            btCollisionShape *shape = new btCapsuleShape(x, y);

            btVector3 inertia;
            btScalar  mass = 1.0;
            shape->calculateLocalInertia(mass, inertia);
            btRigidBody *rb = new btRigidBody(
                btRigidBody::btRigidBodyConstructionInfo(
                mass, new btDefaultMotionState( world, local ), shape, inertia) );
            dynamicWorld->addRigidBody(rb);
            rb->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
        }
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

        mat4 mat;
        Instance insta;
        btTransform pose = body->getWorldTransform();
        pose.getOpenGLMatrix(&mat[0][0]);
        insta.pose = mat3x4(transpose(mat));

        switch (shape->getShapeType()) {
        case STATIC_PLANE_PROXYTYPE:
            insta.par = vec4(d1,0,0,intBitsToFloat(-1));
            break;
        case BOX_SHAPE_PROXYTYPE:
            insta.par = vec4(h1.x(),h1.y(),h1.z(),intBitsToFloat(0));
            break;
        case SPHERE_SHAPE_PROXYTYPE:
            insta.par = vec4(r1,0,0,intBitsToFloat(1));
            break;
        case CAPSULE_SHAPE_PROXYTYPE:
        {
            float h2 = ((btCapsuleShape*)shape)->getHalfHeight();
            insta.par = vec4(h2,r2,0,intBitsToFloat(2));
            break;
        }
        case CYLINDER_SHAPE_PROXYTYPE:
        {
            float r3 = ((btCylinderShape*)shape)->getRadius();
            insta.par = vec4(h2,r3,0,intBitsToFloat(3));
            break;
        }
        case CONE_SHAPE_PROXYTYPE:
            insta.par = vec4(h3,r4,0,intBitsToFloat(4));
            break;
        }

        I.push_back(insta);
    }
}
