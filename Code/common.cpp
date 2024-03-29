#include <assert.h>
#include <stdio.h>
#include <glad/glad.h>
#include <sys/stat.h>
#include <dlfcn.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>
using namespace glm;
#include <LinearMath/btAlignedObjectArray.h>
template <typename T> using myList = btAlignedObjectArray<T>;

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

unsigned int loadTexture1(const char *filename)
{
    GLuint tex;
    int w, h, c;
    stbi_uc *data = stbi_load(filename, &w,&h,&c, STBI_grey_alpha);
    if (!data)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return -1;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG8, w,h);
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,w,h, GL_RG, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return tex;
}

bool recordVideo(int nFrame)
{
    static const char fmt[] = "$HOME/.spotdl/ffmpeg"
            " -r 40 -f rawvideo -pix_fmt rgb24 -s %dx%d"
            " -i pipe: -c:v libx264 -c:a aac"
            " -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip 1.mp4";

    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    const int resX = vp[2] - vp[0];
    const int resY = vp[3] - vp[1];

    static char *cmd = new char[strlen(fmt) + 16];
    sprintf(cmd, fmt, resX, resY);
    static FILE *pipe = popen(cmd, "w");
    static int frame = 0;
    static char *buffer = new char[resX*resY*3];

    glReadPixels(0, 0, resX, resY, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    fwrite(buffer, resX*resY*3, 1, pipe);
    if (frame++ > nFrame)
    {
        pclose(pipe);
        return false;
    }
    return true;
}

/************************************************************
 *                        System                            *
************************************************************/

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

myList<int> loadFnt(const char *filename)
{
    myList<int> fontinfo;
    fontinfo.resize(128*7, 0);
    FILE *file = fopen(filename, "r");
    if (file)
    {
        fscanf(file, "%*[^\n]\n%*[^\n]\n%*[^\n]\n%*[^\n]\n");
        for (;;)
        {
            long cur = ftell(file);
            int a,b,c,d,e,f,g,h;
            int eof = fscanf(file, "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d"
                                   "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d",
                &a,&b,&c,&d,&e,&f,&g,&h);
            if (eof < 0) break;
            fseek(file, cur, SEEK_SET);
            fscanf(file, "%*[^\n]\n");

            fontinfo[a*7+0] = b;
            fontinfo[a*7+1] = c;
            fontinfo[a*7+2] = d;
            fontinfo[a*7+3] = e;
            fontinfo[a*7+4] = f;
            fontinfo[a*7+5] = g;
            fontinfo[a*7+6] = h;
        }
        fclose(file);
        printf("INFO: loaded file %s\n", filename);
    }
    return fontinfo;
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

/************************************************************
 *                       DebugDraw                          *
************************************************************/

#include "common.h"

myDebugDraw::myDebugDraw(const char *filename)
{
    _fontInfo = loadFnt(filename);
    assert(_fontInfo.size());
}

void myDebugDraw::drawRectangle(btVector4 dest, btVector4 source)
{
    _quadBuffer <<
            dest.x(), dest.y(), dest.z(), dest.w(),
            source.x(), source.y(), source.z(), source.w() * -1;
}

void myDebugDraw::draw2dText(float xloc, float yloc, float fontSize, const char *format...)
{
    const float spacing = .8;

    char textString[128];
    va_list args;
    va_start(args, format);
    vsprintf(textString, format, args);
    va_end(args);

    const int spaceXadv = _fontInfo[int('_')*7+6];
    fontSize /= spaceXadv * 1.8;

    float xpos = 0., ypos = 0.;
    const int nChars = strlen(textString);
    for (int i=0; i<nChars; i++)
    {
        const char id = textString[i];
        int x    = _fontInfo[id*7+0];
        int y    = _fontInfo[id*7+1];
        int w    = _fontInfo[id*7+2];
        int h    = _fontInfo[id*7+3];
        int xoff = _fontInfo[id*7+4];
        int yoff = _fontInfo[id*7+5];
        int xadv = _fontInfo[id*7+6];

        switch (id) {
        case ' ':
            xadv = spaceXadv;
            break;
        case '\t':
            xadv = spaceXadv * 5;
            break;
        case '\n':
            xadv = 0,
            xpos = 0.0f,
            ypos += fontSize * spaceXadv * 1.8 * spacing;
            break;
        default:
            _quadBuffer <<
                    xloc + xpos + xoff*fontSize,
                    yloc + ypos + yoff*fontSize,
                    w*fontSize, h*fontSize, x, y, w, h;
            break;
        }

        xpos += fontSize * xadv * spacing;
    }
}
