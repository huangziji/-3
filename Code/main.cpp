#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"

static void error_callback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

int main()
{
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

        int triCount = 0;
        {
            void *f = loadPlugin("libBvhPlugin.so", "mainAnimation");
            typedef int (plugin)(float);
            if (f) triCount = ((plugin*)f)(time);
        }

        static GLuint prog1 = glCreateProgram();
        static GLuint prog2 = glCreateProgram();
        {
            static int iTime1;// iTime2;
            static long lastModTime1;// lastModTime2;
            // bool dirty2 = reloadShader2(&lastModTime2, prog2, "../Code/base.glsl");
            // if (dirty2)
            // {
            //     iTime2 = glGetUniformLocation(prog2, "iTime");
            //     int iResolution = glGetUniformLocation(prog2, "iResolution");
            //     glProgramUniform2f(prog2, iResolution, RES_X, RES_Y);
            // }
            // glProgramUniform1f(prog2, iTime2, time);
            bool dirty1 = reloadShader1(&lastModTime1, prog1, "../Code/base.frag");
            if (dirty1)
            {
                iTime1 = glGetUniformLocation(prog1, "iTime");
                int iResolution = glGetUniformLocation(prog1, "iResolution");
                glProgramUniform2f(prog1, iResolution, RES_X, RES_Y);
                int iChannel1 = glGetUniformLocation(prog1, "iChannel1");
                int iChannel2 = glGetUniformLocation(prog1, "iChannel2");
                glProgramUniform1i(prog1, iChannel1, 1);
                glProgramUniform1i(prog1, iChannel2, 2);
            }
            glProgramUniform1f(prog1, iTime1, time);
        }

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window1);
        glfwPollEvents();

        // if (!recordVideo(5.39)) break;
    }

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
}
