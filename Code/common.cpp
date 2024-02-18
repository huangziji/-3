#include <assert.h>
#include <stdio.h>
#include <glad/glad.h>
#include <sys/stat.h>
#include <dlfcn.h>

/************************************************************
 *                     GL Utilities                         *
************************************************************/

static int loadShader1(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char version[32];
    fgets(version, sizeof(version), f);
    length -= ftell(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

    GLuint fs, vs;
    {
        const char *string[] = { version, source };
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, sizeof string/sizeof *string, string, NULL);
        glCompileShader(fs);
        int success;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            int length;
            glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);
            char message[length];
            glGetShaderInfoLog(fs, length, &length, message);
            glDeleteShader(fs);
            fprintf(stderr, "ERROR: fail to compile fragment shader. file %s\n%s\n", filename, message);
            return 1;
        }
    }
    {
        const char vsSource[] = R"(
        precision mediump float;
        void main() {
            vec2 UV = vec2(gl_VertexID%2, gl_VertexID/2)*2.-1.;
            gl_Position = vec4(UV, 0, 1);
        }
        )";
        const char *string[] = { version, vsSource };
        vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 2, string, NULL);
        glCompileShader(vs);
        int success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        assert(success);
    }

    GLsizei NbShaders;
    GLuint shaders[2];
    glGetAttachedShaders(prog, 2, &NbShaders, shaders);
    for (int i=0; i<NbShaders; i++)
    {
        glDeleteShader(shaders[i]);
        glDetachShader(prog, shaders[i]);
    }
    glAttachShader(prog, fs);
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}

static int loadShader2(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char version[32];
    fgets(version, sizeof(version), f);
    length -= ftell(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

    GLuint shd[2];
    for (int i=0; i<2; i++)
    {
        const char *string[] = { version, i==0?"#define _VS\n":"#define _FS\n", source };
        const GLuint fs = glCreateShader(i==0?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
        glShaderSource(fs, sizeof string/sizeof *string, string, NULL);
        glCompileShader(fs);
        int success;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            int length;
            glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);
            char message[length];
            glGetShaderInfoLog(fs, length, &length, message);
            glDeleteShader(fs);
            fprintf(stderr, "ERROR: fail to compile fragment shader. file %s\n%s\n", filename, message);
            return 1;
        }
        shd[i] = fs;
    }

    GLsizei NbShaders;
    GLuint shaders[2];
    glGetAttachedShaders(prog, 2, &NbShaders, shaders);
    for (int i=0; i<NbShaders; i++)
    {
        glDeleteShader(shaders[i]);
        glDetachShader(prog, shaders[i]);
    }
    glAttachShader(prog, shd[0]);
    glAttachShader(prog, shd[1]);
    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}

bool reloadShader0(typeof loadShader1 f, long *lastModTime, GLuint prog, const char *filename)
{
    struct stat libStat;
    int err = stat(filename, &libStat);
    if (err == 0 && *lastModTime != libStat.st_mtime)
    {
        err = f(prog, filename);
        if (err >= 0)
        {
            printf("INFO: reloading file %s\n", filename);
            *lastModTime = libStat.st_mtime;
            return true;
        }
    }
    return false;
}

bool reloadShader1(long *lastModTime, GLuint prog, const char *filename)
{
    return reloadShader0(loadShader1, lastModTime, prog, filename);
}

bool reloadShader2(long *lastModTime, GLuint prog, const char *filename)
{
    return reloadShader0(loadShader2, lastModTime, prog, filename);
}

void *loadPlugin(const char * filename, const char *funcname)
{
    static long lastModTime;
    static void *handle = NULL;
    static void *f = NULL;

    struct stat libStat;
    int err = stat(filename, &libStat);
    if (err == 0 && lastModTime != libStat.st_mtime)
    {
        if (handle)
        {
            assert(dlclose(handle) == 0);
        }
        handle = dlopen(filename, RTLD_NOW);
        if (handle)
        {
            printf("INFO: reloading file %s\n", filename);
            lastModTime = libStat.st_mtime;
            f = dlsym(handle, funcname);
            assert(f);
        }
    }
    return f;
}
