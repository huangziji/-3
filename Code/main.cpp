#include "common.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <btBulletDynamicsCommon.h>
template <typename T> using vector = btAlignedObjectArray<T>;
#include <miniaudio.h>

class myDebugDraw : public btIDebugDraw
{
    int _debugMode;
public:
    vector<btVector3> _lineBuffer;

    void clearLines() override final { _lineBuffer.clear(); }
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override final
    {
        _lineBuffer.push_back(to);
        _lineBuffer.push_back(from);
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

    const int RES_X = 16*50, RES_Y = 9*50;
    GLFWwindow *window1;
    { // window1
        int screenWidth, screenHeight;
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        glfwGetMonitorWorkarea(primaryMonitor, NULL, NULL, &screenWidth, &screenHeight);

        window1 = glfwCreateWindow(RES_X, RES_Y, "#1", NULL, NULL);
        glfwMakeContextCurrent(window1);
        glfwSetWindowPos(window1, screenWidth-RES_X, 0);
        glfwSetWindowAttrib(window1, GLFW_FLOATING, GLFW_TRUE);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
       // glfwSwapInterval(0);
    }

    long lastModTime3;
    GLuint prog3 = glCreateProgram();
    GLuint tex = loadTexture1("../Data/arial.png");

    while (!glfwWindowShouldClose(window1))
    {
        loadShader2(&lastModTime3, prog3, "../Code/text.glsl");

        double xpos, ypos;
        glfwGetCursorPos(window1, &xpos, &ypos);
        float time = glfwGetTime();

        float ti = xpos / RES_X * M_PI * 2. - 1.;//time;
        btVector3 ta = btVector3(0,1,0);
        btVector3 ro = ta + btVector3(sin(ti),ypos/RES_Y * 3. - .5,cos(ti)).normalize() * 2.f;

        vector<btVector3> const& V = dd->_lineBuffer;
        vector<float> U;
        vector<float> I;

        {
            void *f = loadPlugin("libRagdollPlugin.so", "mainAnimation");
            typedef void (plugin)(vector<float> &, vector<float> &, btDynamicsWorld*, float);
            if (f) ((plugin*)f)(I, U, dynamicWorld, time);

            static GLuint vbo1, vbo2, ubo1, ssbo1, frame = 0;
            if (frame++ == 0)
            {
                glGenBuffers(1, &vbo1);
                glGenBuffers(1, &vbo2);
                glGenBuffers(1, &ubo1);
                glGenBuffers(1, &ssbo1);
            }

            glBindBuffer(GL_ARRAY_BUFFER, vbo1);
            glBufferData(GL_ARRAY_BUFFER, sizeof V[0] * V.size(), &V[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, 0);

            glBindBuffer(GL_ARRAY_BUFFER, vbo2);
            glBufferData(GL_ARRAY_BUFFER, sizeof U[0] * U.size(), &U[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 32, 0);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 32, (void*)16);
            glVertexAttribDivisor(1, 1);
            glVertexAttribDivisor(2, 1);

            int data[] = { (int)I.size()/16 };
            glBindBuffer(GL_UNIFORM_BUFFER, ubo1);
            glBufferData(GL_UNIFORM_BUFFER, sizeof data, data, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo1);

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof I[0] * I.size(), &I[0], GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo1);
        }

        static GLuint prog1 = glCreateProgram();
        static GLuint prog2 = glCreateProgram();
        {
            static long lastModTime1, lastModTime2;
            loadShader1(&lastModTime1, prog1, "../Code/base.frag");
            loadShader2(&lastModTime2, prog2, "../Code/base.glsl");
            const float data[] = { ta.x(), ta.y(), ta.z(), ro.x(), ro.y(), ro.z() };
            glProgramUniform2f(prog1, 0, RES_X, RES_Y);
            glProgramUniform2f(prog2, 0, RES_X, RES_Y);
            glProgramUniformMatrix2x3fv(prog1, 1, 1, GL_FALSE, data);
            glProgramUniformMatrix2x3fv(prog2, 1, 1, GL_FALSE, data);
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
        glDrawArrays(GL_LINES, 0, V.size());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUseProgram(prog3);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, U.size()/6);

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
