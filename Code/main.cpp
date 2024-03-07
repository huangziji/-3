#include "common.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <btBulletDynamicsCommon.h>
template <typename T> using myList = btAlignedObjectArray<T>;
#include <miniaudio.h>

static myList<float> &operator<<(myList<float> &a, float b)
{
    a.push_back(b);
    return a;
}
static myList<float> &operator,(myList<float> &a, float b) { return a << b; }

class myDebugDraw : public btIDebugDraw
{
    int _debugMode;
public:
    myList<float> _lineBuffer;

    void clearLines() override final { _lineBuffer.clear(); }
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
        return DBG_DrawConstraints;//|DBG_DrawWireframe;
    }
};

static void error_callback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

int main()
{
    ma_result result;
    ma_engine engine;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        return -1;
    }

    ma_engine_play_sound(&engine, "../Data/waterdrop24.wav", NULL);

    myDebugDraw *dd = new myDebugDraw;
    btCollisionConfiguration *conf = new btDefaultCollisionConfiguration;
    btDynamicsWorld *dynamicWorld = new btDiscreteDynamicsWorld(
                new btCollisionDispatcher(conf), new btDbvtBroadphase,
                new btSequentialImpulseConstraintSolver, conf);
    dynamicWorld->setDebugDrawer(dd);

    glfwInit();
    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window1;
    { // window1
        const int RES_X = 16*50, RES_Y = 9*50;
        window1 = glfwCreateWindow(RES_X, RES_Y, "Ragdoll Demo", NULL, NULL);
        glfwMakeContextCurrent(window1);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        // set window properties
        int screenWidth, screenHeight;
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        glfwGetMonitorWorkarea(primaryMonitor, NULL, NULL, &screenWidth, &screenHeight);
        glfwSetWindowPos(window1, screenWidth-RES_X, 0);
        glfwSetWindowAttrib(window1, GLFW_FLOATING, GLFW_TRUE);
    }

    while (!glfwWindowShouldClose(window1))
    {
        typedef struct { int x,y; } ivec2;
        typedef struct { int x,y,z,w; } ivec4;

        const float iTime = glfwGetTime();
        static int iFrame = -1;

        ivec2 iResolution;
        double posX, posY;
        glfwGetWindowSize(window1, &iResolution.x, &iResolution.y);
        glfwGetCursorPos(window1, &posX, &posY);
        static float lastFrameTime = 0;
        const float iTimeDelta = iTime - lastFrameTime;
        lastFrameTime = iTime;
        iFrame += 1;

        ivec4 iMouse = { (int)posX, (int)posY };

        typedef struct { myList<float> viewBuffer, meshBuffer, textBuffer; }Varying;
        typedef Varying (plugin)(btDynamicsWorld*,
            ivec2 iResolution, float iTime, float iTimeDelta, float iFrame, ivec4 iMouse
                                 );

        void *f = loadPlugin("libRagdollPlugin.so", "mainAnimation");
        const Varying res = f ? ((plugin*)f)(dynamicWorld, iResolution, iTime, iTimeDelta, 0, iMouse) : Varying{};

        const myList<float> & V1 = dd->_lineBuffer;
        const myList<float> & V2 = res.textBuffer;
        const myList<float> & U1 = res.viewBuffer;
        const myList<float> & W1 = res.meshBuffer;
        const int nLines = V1.size() / 4;
        const int nQuads = V2.size() / 8;
        const int nMeshes = W1.size() / 16;

        { // buffers
            static GLuint vbo1, vbo2, ubo1, ssbo1, frame = 0;
            if (frame++ == 0)
            {
                glGenBuffers(1, &vbo1);
                glGenBuffers(1, &vbo2);
                glGenBuffers(1, &ubo1);
                glGenBuffers(1, &ssbo1);
            }

            glBindBuffer(GL_ARRAY_BUFFER, vbo1);
            glBufferData(GL_ARRAY_BUFFER, sizeof V1[0] * V1.size(), &V1[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, 0);

            glBindBuffer(GL_ARRAY_BUFFER, vbo2);
            glBufferData(GL_ARRAY_BUFFER, sizeof V2[0] * V2.size(), &V2[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 32, 0);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 32, (void*)16);
            glVertexAttribDivisor(1, 1);
            glVertexAttribDivisor(2, 1);

            glBindBuffer(GL_UNIFORM_BUFFER, ubo1);
            glBufferData(GL_UNIFORM_BUFFER, sizeof U1[0] * U1.size(), &U1[0], GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo1);

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof W1[0] * W1.size(), &W1[0], GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo1);
        }

        static GLuint tex = loadTexture1("../Data/arial.png");
        { // texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
        }

        static GLuint prog1 = glCreateProgram();
        static GLuint prog2 = glCreateProgram();
        static GLuint prog3 = glCreateProgram();
        { // programs
            static time_t lastModTime1, lastModTime2, lastModTime3;
            loadShader1(&lastModTime1, prog1, "../Code/base.frag");
            loadShader2(&lastModTime2, prog2, "../Code/base.glsl");
            loadShader2(&lastModTime3, prog3, "../Code/text.glsl");
            glProgramUniform2f(prog1, 0, iResolution.x, iResolution.y);
            glProgramUniform2f(prog2, 0, iResolution.x, iResolution.y);
            glProgramUniform1i(prog1, 5, nMeshes);
        }

        glDepthMask(1);
        glFrontFace(GL_CCW);
        glDepthFunc(GL_LESS);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDepthMask(0);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(prog2);
        glDrawArrays(GL_LINES, 0, nLines);
        glUseProgram(prog3);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, nQuads);

        glfwSwapBuffers(window1);
        glfwPollEvents();
        // if (!recordVideo(5.39)) break;
    }

    ma_engine_uninit(&engine);
    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
}
