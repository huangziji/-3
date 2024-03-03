#include <assert.h>
#include <stdio.h>
#include <glad/glad.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <glm/glm.hpp>
using namespace glm;

/************************************************************
 *                      Hotloading                          *
************************************************************/

static int loadShader_static(int N, GLuint prog, const char *filename)
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

    const char vsSource[] = R"(
    precision mediump float;
    void main() {
        vec2 UV = vec2(gl_VertexID%2, gl_VertexID/2)*2.-1.;
        gl_Position = vec4(UV, 0, 1);
    }
    )";

    GLuint newShader[2];
    for (int i=0; i<2; i++)
    {
        const char *string[] = { version,
                                 i == 0 ? "#define _FS\n" : "#define _VS\n",
                                 (i == 0 || N == 2) ? source : vsSource };
        GLuint fs = glCreateShader(i == 0 ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
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
            fprintf(stderr, "ERROR: fail to compile %s shader. file %s\n%s\n",
                    i == 0 ? "fragment" : "vertex", filename, message);
            return 1;
        }
        newShader[i] = fs;
    }

    GLsizei NbShaders;
    GLuint oldShader[2];
    glGetAttachedShaders(prog, 2, &NbShaders, oldShader);
    for (int i=0; i<NbShaders; i++)
    {
        glDetachShader(prog, oldShader[i]);
        glDeleteShader(oldShader[i]);
    }
    glAttachShader(prog, newShader[0]);
    glAttachShader(prog, newShader[1]);
    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}

static bool loadShader_dynamic(int N, long *lastModTime, GLuint prog, const char *filename)
{
    struct stat libStat;
    int err = stat(filename, &libStat);
    if (err == 0 && *lastModTime != libStat.st_mtime)
    {
        err = loadShader_static(N, prog, filename);
        if (err >= 0)
        {
            printf("INFO: reloading file %s\n", filename);
            *lastModTime = libStat.st_mtime;
            return true;
        }
    }
    return false;
}

bool loadShader1(long *lastModTime, GLuint prog, const char *filename)
{
    return loadShader_dynamic(1, lastModTime, prog, filename);
}

bool loadShader2(long *lastModTime, GLuint prog, const char *filename)
{
    return loadShader_dynamic(2, lastModTime, prog, filename);
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
        else
        {
            f = NULL;
        }
    }
    return f;
}

bool recordVideo(float sec)
{
    const int resX = 800, resY = 450;
    static const char cmd[] = "$HOME/.spotdl/ffmpeg"
            " -r 60 -f rawvideo -pix_fmt rgb24 -s 800x450"
            " -i pipe: -c:v libx264 -c:a aac"
            " -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip 1.mp4";

    static FILE *pipe = popen(cmd, "w");
    static char *buffer = new char[resX*resY*3];
    static int frame = 0;

    glReadPixels(0,0, resX, resY, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    fwrite(buffer, resX*resY*3, 1, pipe);
    if (frame++ > sec*60)
    {
        pclose(pipe);
        return false;
    }
    return true;
}

/************************************************************
 *                       Utilities                          *
************************************************************/

/// @link https://iquilezles.org/articles/noacos/
mat3x3 rotationAlign( vec3 d, vec3 z )
{
    vec3  v = cross( z, d );
    float c = dot( z, d );
    float k = 1.0f/(1.0f+c);

    return mat3x3( v.x*v.x*k + c,     v.y*v.x*k - v.z,    v.z*v.x*k + v.y,
                   v.x*v.y*k + v.z,   v.y*v.y*k + c,      v.z*v.y*k - v.x,
                   v.x*v.z*k - v.y,   v.y*v.z*k + v.x,    v.z*v.z*k + c    );
}

/// @link https://iquilezles.org/articles/simpleik/
vec3 solve( vec3 p, float r1, float r2, vec3 dir )
{
    vec3 q = p*( 0.5f + 0.5f*(r1*r1-r2*r2)/dot(p,p) );

    float s = r1*r1 - dot(q,q);
    s = max( s, 0.0f );
    q += sqrt(s)*normalize(cross(p,dir));

    return q;
}

static mat4 coefs = {
     0, 2, 0, 0,
    -1, 0, 1, 0,
     2,-5, 4,-1,
    -1, 3,-3, 1 };

/// @source: Texturing And Modeling A Procedural Approach
float spline( const float *k, int n, float t )
{
    t *= n - 3;
    int i = int(floor(t));
    t -= i;
    k += i;

    vec4 f = (coefs * .5f) * vec4(1, t, t*t, t*t*t);
    vec4 j = *(vec4*)k;
    return dot(f, j);
}
