#ifndef COMMON_H
#define COMMON_H

bool loadShader1(long *, unsigned int, const char *);

bool loadShader2(long *, unsigned int, const char *);

unsigned int loadTexture1(const char *filename);

void *loadPlugin(const char *, const char *);

bool recordVideo(int nFrame);

float spline( const float *k, int n, float t );

#include <LinearMath/btIDebugDraw.h>
#include <LinearMath/btAlignedObjectArray.h>
template <typename T> using myList = btAlignedObjectArray<T>;

myList<int> loadFnt(const char *filename);

inline myList<float> &operator<<(myList<float> &a, float b)
{
    a.push_back(b);
    return a;
}

inline myList<float> &operator,(myList<float> &a, float b)
{
    return a << b;
}

class myDebugDraw : public btIDebugDraw
{
    int _debugMode;
    myList<int> _fontInfo;
public:
    myList<float> _lineBuffer;
    myList<float> _quadBuffer;

    explicit myDebugDraw(const char *filename);

    void clearLines() override final
    {
        _lineBuffer.clear();
        _quadBuffer.clear();
    }

    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override final
    {
        _lineBuffer <<
                from.x(), from.y(), from.z(), 0,
                to.x(), to.y(), to.z(), 0;
    }

    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override final {}
    void reportErrorWarning(const char* warningString) override final
    {
        fprintf(stderr, "ERROR: %s\n", warningString);
    }
    void draw3dText(const btVector3& location, const char* textString)  override final {}
    void setDebugMode(int debugMode)  override final { _debugMode = debugMode; }

    virtual int getDebugMode() const override final
    {
        return DBG_DrawConstraints;
    }

    void draw2dText(float x, float y, const char *textString, float fontSize);

    void drawRectangle(btVector4 dest, btVector4 source);
};

#endif // COMMON_H
