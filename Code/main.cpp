#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>

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

    bool reloadShader1(long *, GLuint , const char *);
    void *loadPlugin(const char *, const char *);

    int iTime;
    long lastModTime;
    GLuint prog1 = glCreateProgram();

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
        {
            void *f = loadPlugin("libBvhPlugin.so", "mainAnimation");
            typedef void (plugin)(void);
            if (f) ((plugin*)f)();
        }
        {
            bool dirty = reloadShader1(&lastModTime, prog1, "../Code/base.frag");
            if (dirty)
            {
                iTime = glGetUniformLocation(prog1, "iTime");
                int iResolution = glGetUniformLocation(prog1, "iResolution");
                glProgramUniform2f(prog1, iResolution, RES_X, RES_Y);
            }
        }

        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog1);
        glProgramUniform1f(prog1, iTime, glfwGetTime());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window1);
        glfwPollEvents();
    }

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
}
