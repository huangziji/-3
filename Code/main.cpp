#include "common.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <miniaudio.h>
#include <btBulletDynamicsCommon.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

static void error_callback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

static void joystick_callback(int jid, int event)
{
    if (event == GLFW_CONNECTED)
    {
        printf("INFO: The joystick %d was connected\n", jid);
    }
    else if (event == GLFW_DISCONNECTED)
    {
        printf("INFO: The joystick %d was disconnected\n", jid);
    }
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

    myDebugDraw *dd = new myDebugDraw("../Data/arial.fnt");
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

    glfwSetJoystickCallback(joystick_callback);
    int isGamepad = glfwJoystickIsGamepad(GLFW_JOYSTICK_1);
    int present = glfwJoystickPresent(GLFW_JOYSTICK_1);
    const char* name = glfwGetJoystickName(GLFW_JOYSTICK_1);
    printf("JoystickName : %s\nPresent : %s\nIs GamePad : %s\n",
           name, present?"true":"false", isGamepad?"true":"false");

    GLuint bufferA, tex5;
    {
        const int size = 9*50;
        glGenTextures(1, &tex5);
        glBindTexture(GL_TEXTURE_2D, tex5);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, size, size);
        glGenFramebuffers(1, &bufferA);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex5, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }

    const GLuint tex4 = loadTexture1("../Data/arial.bmp");

    while (!glfwWindowShouldClose(window1))
    {
        typedef struct { int x,y; } ivec2;
        typedef struct { int x,y,z,w; } ivec4;

        ivec2 iResolution;
        const float iTime = glfwGetTime();
        float iTimeDelta;
        static int iFrame = -1;
        ivec4 iMouse;
        {
            glfwGetFramebufferSize(window1, &iResolution.x, &iResolution.y);
            static float lastFrameTime = 0;
            iTimeDelta = iTime - lastFrameTime;
            lastFrameTime = iTime;
            iFrame += 1;
            double posX, posY;
            glfwGetCursorPos(window1, &posX, &posY);
            iMouse.x = posX, iMouse.y = posY;
        }

        static time_t lastModTime4;
        static const GLuint prog4 = glCreateProgram();
        bool dirty = loadShader1(&lastModTime4, prog4, "../Code/bake2.frag");
        if (dirty)
        {
            const int size = iResolution.y;
            glDisable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
            glViewport(0, 0, size, size);
            glClear(GL_COLOR_BUFFER_BIT);
            glProgramUniform2f(prog4, 0, size, size);
            glUseProgram(prog4);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        typedef struct { myList<float> viewBuffer, meshBuffer; }Varying;
        typedef bool (plugin)(Varying *, btDynamicsWorld*, ivec2 iResolution, float iTime, float iTimeDelta, float iFrame, ivec4 iMouse);

        Varying res;
        void *f = loadPlugin("libRagdollPlugin.so", "mainAnimation");
        if (f) ((plugin*)f)(&res, dynamicWorld, iResolution, iTime, iTimeDelta, 0, iMouse);

#if 0
        { // joystick
            int count;
            char text[128];
            char *p = text;

            const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);
            sprintf(p, "\n%d  ", count); p += 4;
            for (int i=0; i<count; i++)
            {
                sprintf(p, "%2.2f  ", axes[i]); p += 6;
            }

            const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &count);
            sprintf(p, "\n%d  ", count); p += 4;
            for (int i=0; i<count; i++)
            {
                sprintf(p, "%d ", buttons[i]); p += 2;
            }

            const unsigned char* hats = glfwGetJoystickHats(GLFW_JOYSTICK_1, &count);
            sprintf(p, "\n%d  ", count); p += 4;
            for (int i=0; i<count; i++)
            {
                sprintf(p, "%d ", hats[i]); p += 2;
            }

            dd->draw2dText(iResolution.x*.1, iResolution.y*.5, text, 20);
        }
#endif

        const myList<float> & V1 = dd->_lineBuffer;
        const myList<float> & V2 = dd->_quadBuffer;
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

        { // texture
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, tex4);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, tex5);
        }

        static const GLuint prog1 = glCreateProgram();
        static const GLuint prog2 = glCreateProgram();
        static const GLuint prog3 = glCreateProgram();
        { // programs
            static time_t lastModTime1, lastModTime2, lastModTime3;
            loadShader1(&lastModTime1, prog1, "../Code/base.frag");
            loadShader2(&lastModTime2, prog2, "../Code/base.glsl");
            loadShader2(&lastModTime3, prog3, "../Code/text.glsl");
            glProgramUniform2f(prog1, 0, iResolution.x, iResolution.y);
            glProgramUniform2f(prog2, 0, iResolution.x, iResolution.y);
            glProgramUniform2f(prog3, 0, iResolution.x, iResolution.y);
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
        glViewport(0, 0, iResolution.x, iResolution.y);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDepthMask(0);
        glUseProgram(prog2);
        glDrawArrays(GL_LINES, 0, nLines);

        glDisable(GL_DEPTH_TEST);
        glUseProgram(prog3);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, nQuads);

        if (iFrame == 0)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            ImGui_ImplGlfw_InitForOpenGL(window1, true);
            ImGui_ImplOpenGL3_Init("#version 130");
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Hello World");
        ImGui::Text("ss");
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window1);
        glfwPollEvents();
        // if (!recordVideo(0xfff)) break;
    }

    ma_engine_uninit(&engine);
    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
}
