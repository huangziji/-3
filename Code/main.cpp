#include "common.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <btBulletDynamicsCommon.h>

class MyDebugDraw : public btIDebugDraw
{
    int _debugMode;
public:
    btAlignedObjectArray<btVector3> _data;
    void clearLines() override final { _data.clear(); }
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override final
    {
        _data.push_back(to);
        _data.push_back(from);
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
        // return _debugMode;
        return DBG_DrawAabb;//|DBG_DrawWireframe|
                    //DBG_DrawConstraints|DBG_DrawConstraintLimits;
    }
};

static void error_callback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

int main()
{
    MyDebugDraw *dd = new MyDebugDraw;
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

    GLuint bufferA, tex1, tex2;
    {
        glGenTextures(1, &tex1);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, RES_X, RES_Y);
        glGenTextures(1, &tex2);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, RES_X, RES_Y);

        GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
        glGenFramebuffers(1, &bufferA);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex1, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);
        glReadBuffer(GL_NONE);
        glDrawBuffers(1, drawBuffers);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    while (!glfwWindowShouldClose(window1))
    {
        float time = glfwGetTime();
        static uint32_t frame = -1;
        {
            static float fps, lastFrameTime = 0;
            float dt = time - lastFrameTime;
            lastFrameTime = time;
            if ((frame++ & 0xf) == 0) fps = 1./dt;
            char title[32];
            sprintf(title, "%.2f\t\t%.1f fps\t\t%d x %d", time, fps, RES_X, RES_Y);
            glfwSetWindowTitle(window1, title);
            double xpos, ypos;
            glfwGetCursorPos(window1, &xpos, &ypos);
        }

        btAlignedObjectArray<btVector3> const& V = dd->_data;
        {
            btAlignedObjectArray<btTransform> I;
            void *f = loadPlugin("libBvhPlugin.so", "mainAnimation");
            typedef void (plugin)(btAlignedObjectArray<btTransform> &, btDynamicsWorld*, float);
            if (f) ((plugin*)f)(I, dynamicWorld, time);

            static GLuint vbo, ubo1, ssbo1, frame = 0;
            if (frame++ == 0)
            {
                glGenBuffers(1, &vbo);
                glGenBuffers(1, &ubo1);
                glGenBuffers(1, &ssbo1);
            }

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof V[0] * V.size(), &V[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);

            uint data[] = { (uint)I.size() };
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
            vec3 ta = vec3(0,1,0);
            vec3 ro = ta + vec3(sin(time),0.5,cos(time)) * 2.f;
            static int iCamera1, iCamera2;
            static long lastModTime1, lastModTime2;
            bool dirty1 = reloadShader1(&lastModTime1, prog1, "../Code/base.frag");
            if (dirty1)
            {
                iCamera1 = glGetUniformLocation(prog1, "iCamera");
                int iResolution = glGetUniformLocation(prog1, "iResolution");
                glProgramUniform2f(prog1, iResolution, RES_X, RES_Y);
            }
            bool dirty2 = reloadShader2(&lastModTime2, prog2, "../Code/base.glsl");
            if (dirty2)
            {
                iCamera2 = glGetUniformLocation(prog2, "iCamera");
                int iResolution = glGetUniformLocation(prog2, "iResolution");
                glProgramUniform2f(prog2, iResolution, RES_X, RES_Y);
            }
            float data[] = { ta.x, ta.y, ta.z, ro.x, ro.y, ro.z };
            glProgramUniformMatrix2x3fv(prog1, iCamera1, 1, GL_FALSE, data);
            glProgramUniformMatrix2x3fv(prog2, iCamera2, 1, GL_FALSE, data);
        }

        glDepthMask(1);
        glFrontFace(GL_CCW);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDepthMask(0);
        glUseProgram(prog2);
        glDrawArrays(GL_LINES, 0, V.size());

        glfwSwapBuffers(window1);
        glfwPollEvents();

        // if (!recordVideo(5.39)) break;
    }

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
}
