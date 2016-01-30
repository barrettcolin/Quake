/* cb_8to24: 8bit palettized -> 24bit truecolor data conversion using OpenGL 2.0/OpenGL ES 2.0

Inspired by Ryan C. Gordon's implementation for Postal 1
*/
#ifndef CB_8TO24_H__
#define CB_8TO24_H__

#ifdef CB_8TO24_STATIC
#define CB_8TO24_EXTERN static
#else
#ifdef __cplusplus
#define CB_8TO24_EXTERN extern "C"
#else
#define CB_8TO24_EXTERN extern
#endif
#endif

typedef struct
{
    int vidWidth;
    int vidHeight;
    int winWidth;
    int winHeight;
} cb_8to24_ResolutionParams;

typedef void(*cb_8to24_printfFunc)(char *fmt, ...);

CB_8TO24_EXTERN int cb_8to24_Init(const cb_8to24_ResolutionParams *initParams, unsigned char *paletteRGB, cb_8to24_printfFunc printfFunc);

CB_8TO24_EXTERN void cb_8to24_Shutdown(void);

CB_8TO24_EXTERN void cb_8to24_Update(const unsigned char *vidBuffer);

CB_8TO24_EXTERN void cb_8to24_SetPalette(const unsigned char *paletteRGB);

#endif

#if (defined CB_8TO24_GL2_IMPLEMENTATION || defined CB_8TO24_GLES2_IMPLEMENTATION)

#include <string.h>

enum cb_8to24_VertexAttributes
{
    CB_8TO24_VA_POSITION,
    CB_8TO24_VA_TEXCOORD,

    CB_8TO24_NUM_VERTEX_ATTRIBUTES
};

enum cb_8to24_Texture
{
    CB_8TO24_TEX_BUFFER8,
    CB_8TO24_TEX_PALETTE,
    CB_8TO24_TEX_BUFFER32,

    CB_8TO24_NUM_TEXTURES
};

enum cb_8to24_Error
{
    CB_8TO24_OK = 0,

    CB_8TO24_ERR_FRAMEBUFFER,

    CB_8TO24_ERR_VERTEX_SHADER,
    CB_8TO24_ERR_CONVERT_FRAGMENT_SHADER,
    CB_8TO24_ERR_UPSCALE_FRAGMENT_SHADER,

    CB_8TO24_ERR_CONVERT_PROGRAM,
    CB_8TO24_ERR_UPSCALE_PROGRAM
};

typedef struct
{
    GLfloat x, y, s, t;
} cb_8to24_Vertex;

// -1,1/0,1   1,1/1,1
//   x---------x
//   |         |
//   .         .
//   |         |
//   x---------x
// -1,-1/0,0  1,-1/1,0
static const cb_8to24_Vertex CB_8TO24_VERTS[] =
{
    { -1, 1, 0, 1 },
    { 1, 1, 1, 1 },
    { -1,-1, 0, 0 },
    { 1,-1, 1, 0 },
};

#ifdef CB_8TO24_GL2_IMPLEMENTATION
static const GLbyte CB_8TO24_VS_SRC[] =
"#version 110\n"
"attribute vec2 position;\n"
"attribute vec2 texCoord;\n"
"void main() {\n"
"  gl_Position = vec4(position.xy, 0.0, 1.0);\n"
"  gl_TexCoord[0].xy = texCoord;\n"
"}\n";

static const GLbyte CB_8TO24_CONVERT_FS_SRC[] =
"#version 110\n"
"uniform sampler2D image;\n"
"uniform sampler2D palette;\n"
"void main() {\n"
"  gl_FragColor = texture2D(palette, vec2(texture2D(image, vec2(gl_TexCoord[0].x, 1.0 - gl_TexCoord[0].y)).a, 0));\n"
"}\n";

static const GLbyte CB_8TO24_UPSCALE_FS_SRC[] =
"#version 110\n"
"uniform sampler2D image;\n"
"void main() {\n"
"  gl_FragColor = vec4(texture2D(image, gl_TexCoord[0].xy).rgb, 1.0);\n"
"}\n";
#endif

#ifdef CB_8TO24_GLES2_IMPLEMENTATION
static const GLbyte CB_8TO24_VS_SRC[] =
"attribute vec2 position;\n"
"attribute vec2 texCoord;\n"
"varying vec2 v_texCoord;\n"
"void main() {\n"
"  gl_Position = vec4(position.xy, 0.0, 1.0);\n"
"  v_texCoord = texCoord;\n"
"}\n";

static const GLbyte CB_8TO24_CONVERT_FS_SRC[] =
"precision mediump float;\n"
"uniform sampler2D image;\n"
"uniform sampler2D palette;\n"
"varying vec2 v_texCoord;\n"
"void main() {\n"
"  gl_FragColor = texture2D(palette, vec2(texture2D(image, vec2(v_texCoord.x, 1.0 - v_texCoord.y)).a, 0));\n"
"}\n";

static const GLbyte CB_8TO24_UPSCALE_FS_SRC[] =
"precision mediump float;\n"
"uniform sampler2D image;\n"
"varying vec2 v_texCoord;\n"
"void main() {\n"
"  gl_FragColor = vec4(texture2D(image, v_texCoord.xy).rgb, 1.0);\n"
"}\n";
#endif

typedef struct cb_8to24_State cb_8to24_State;

static struct cb_8to24_State
{
    cb_8to24_printfFunc printfFunc;
    GLsizei vidWidth, vidHeight;
    GLint vpX, vpY;
    GLsizei vpWidth, vpHeight;
    GLuint textures[CB_8TO24_NUM_TEXTURES];
    GLuint frameBuffer;
    GLuint vertexBufferObject;
    GLuint vertexShader;
    GLuint convertFragmentShader;
    GLuint upscaleFragmentShader;
    GLuint convertProgram; // vertexShader/convertFragmentShader
    GLuint upscaleProgram; // vertexShader/upscaleFragmentShader
} cb_8to24__state;

static void cb_8to24__InitTextures(unsigned char *paletteRGB)
{
    int i;

    glGenTextures(CB_8TO24_NUM_TEXTURES, cb_8to24__state.textures);

    for (i = 0; i < CB_8TO24_NUM_TEXTURES; ++i)
    {
        int MIN_FILTER = (i == CB_8TO24_TEX_BUFFER32) ? GL_LINEAR : GL_NEAREST;
        int MAG_FILTER = (i == CB_8TO24_TEX_BUFFER32) ? GL_LINEAR : GL_NEAREST;

        glBindTexture(GL_TEXTURE_2D, cb_8to24__state.textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MIN_FILTER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MAG_FILTER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        switch (i)
        {
        case CB_8TO24_TEX_BUFFER8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, cb_8to24__state.vidWidth, cb_8to24__state.vidHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
            break;

        case CB_8TO24_TEX_PALETTE:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, paletteRGB);
            break;

        case CB_8TO24_TEX_BUFFER32:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cb_8to24__state.vidWidth, cb_8to24__state.vidHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            break;
        }
    }
}

static GLenum cb_8to24__InitFramebuffer(void)
{
    glGenFramebuffers(1, &cb_8to24__state.frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, cb_8to24__state.frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cb_8to24__state.textures[CB_8TO24_TEX_BUFFER32], 0);

    return glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

static void cb_8to24__InitVertexBuffer(void)
{
    int i;

    // Enable vertex attributes
    for (i = 0; i < CB_8TO24_NUM_VERTEX_ATTRIBUTES; ++i)
        glEnableVertexAttribArray(i);

    // Create buffer
    glGenBuffers(1, &cb_8to24__state.vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, cb_8to24__state.vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CB_8TO24_VERTS), CB_8TO24_VERTS, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(CB_8TO24_VA_POSITION, 2, GL_FLOAT, GL_FALSE,
        sizeof(cb_8to24_Vertex), (void *)offsetof(cb_8to24_Vertex, x));
    glVertexAttribPointer(CB_8TO24_VA_TEXCOORD, 2, GL_FLOAT, GL_FALSE,
        sizeof(cb_8to24_Vertex), (void *)offsetof(cb_8to24_Vertex, s));
}

static GLuint cb_8to24__LoadShader(GLenum type, const char *shaderSrc)
{
    GLint compiled;
    GLuint shader = glCreateShader(type);

    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);

    // Compile the shader
    glCompileShader(shader);

    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1 && cb_8to24__state.printfFunc)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            cb_8to24__state.printfFunc("LoadShader: %s\n", infoLog);
            free(infoLog);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint cb_8to24__CreateProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLint linked;
    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    // Bind attributes
    glBindAttribLocation(program, 0, "position");
    glBindAttribLocation(program, 1, "texCoord");

    // Link the program
    glLinkProgram(program);

    // Check the link status
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1 && cb_8to24__state.printfFunc)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            cb_8to24__state.printfFunc("CreateProgram: %s\n", infoLog);
            free(infoLog);
        }

        glDeleteProgram(program);
        return 0;
    }

    return program;
}

static int cb_8to24__InitPrograms()
{
    // Load the vertex/fragment shaders
    cb_8to24__state.vertexShader = cb_8to24__LoadShader(GL_VERTEX_SHADER, CB_8TO24_VS_SRC);
    if (cb_8to24__state.vertexShader == 0)
        return CB_8TO24_ERR_VERTEX_SHADER;

    cb_8to24__state.convertFragmentShader = cb_8to24__LoadShader(GL_FRAGMENT_SHADER, CB_8TO24_CONVERT_FS_SRC);
    if (cb_8to24__state.vertexShader == 0)
        return CB_8TO24_ERR_CONVERT_FRAGMENT_SHADER;

    cb_8to24__state.upscaleFragmentShader = cb_8to24__LoadShader(GL_FRAGMENT_SHADER, CB_8TO24_UPSCALE_FS_SRC);
    if (cb_8to24__state.vertexShader == 0)
        return CB_8TO24_ERR_UPSCALE_FRAGMENT_SHADER;

    // Create programs
    cb_8to24__state.convertProgram = cb_8to24__CreateProgram(cb_8to24__state.vertexShader, cb_8to24__state.convertFragmentShader);
    if (cb_8to24__state.convertProgram == 0)
        return CB_8TO24_ERR_CONVERT_PROGRAM;

    cb_8to24__state.upscaleProgram = cb_8to24__CreateProgram(cb_8to24__state.vertexShader, cb_8to24__state.upscaleFragmentShader);
    if (cb_8to24__state.convertProgram == 0)
        return CB_8TO24_ERR_UPSCALE_PROGRAM;

    glUseProgram(cb_8to24__state.convertProgram);
    glUniform1i(glGetUniformLocation(cb_8to24__state.convertProgram, "image"), 0);
    glUniform1i(glGetUniformLocation(cb_8to24__state.convertProgram, "palette"), 1);

    glUseProgram(cb_8to24__state.upscaleProgram);
    glUniform1i(glGetUniformLocation(cb_8to24__state.upscaleProgram, "image"), 0);

    return CB_8TO24_OK;
}

CB_8TO24_EXTERN int cb_8to24_Init(const cb_8to24_ResolutionParams *resParams, unsigned char *paletteRGB, cb_8to24_printfFunc printfFunc)
{
    int res = CB_8TO24_ERR_FRAMEBUFFER;

    cb_8to24__state.printfFunc = printfFunc;

    cb_8to24__state.vidWidth = resParams->vidWidth;
    cb_8to24__state.vidHeight = resParams->vidHeight;

    // Viewport size and location
    if (resParams->winWidth >= resParams->winHeight * (4.0f / 3.0f))
    {
        cb_8to24__state.vpWidth = (GLsizei)(resParams->winHeight * (4.0f / 3.0f));
        cb_8to24__state.vpHeight = resParams->winHeight;
    }
    else
    {
        cb_8to24__state.vpWidth = resParams->winWidth;
        cb_8to24__state.vpHeight = (GLsizei)(resParams->winWidth * (3.0f / 4.0f));
    }
    cb_8to24__state.vpX = (GLint)((resParams->winWidth - cb_8to24__state.vpWidth) * 0.5f);
    cb_8to24__state.vpY = (GLint)((resParams->winHeight - cb_8to24__state.vpHeight) * 0.5f);

    // Disable depth read/write
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Textures
    cb_8to24__InitTextures(paletteRGB);

    // Vertex Buffer
    cb_8to24__InitVertexBuffer();

    for (;;)
    {
        // Framebuffer
        if (cb_8to24__InitFramebuffer() != GL_FRAMEBUFFER_COMPLETE)
            break;

        // Programs
        res = cb_8to24__InitPrograms();
        if (res != CB_8TO24_OK)
            break;

        return CB_8TO24_OK;
    }

    cb_8to24_Shutdown();
    return res;
}

CB_8TO24_EXTERN void cb_8to24_Shutdown(void)
{
    int i;
    for (i = 0; i < CB_8TO24_NUM_VERTEX_ATTRIBUTES; ++i)
        glDisableVertexAttribArray(i);

    glDeleteBuffers(1, &cb_8to24__state.vertexBufferObject);

    glDeleteProgram(cb_8to24__state.upscaleProgram);
    glDeleteProgram(cb_8to24__state.convertProgram);

    glDeleteShader(cb_8to24__state.upscaleFragmentShader);
    glDeleteShader(cb_8to24__state.convertFragmentShader);
    glDeleteShader(cb_8to24__state.vertexShader);

    glDeleteFramebuffers(1, &cb_8to24__state.frameBuffer);

    glDeleteTextures(CB_8TO24_NUM_TEXTURES, cb_8to24__state.textures);

    memset(&cb_8to24__state, 0, sizeof(cb_8to24__state));
}

CB_8TO24_EXTERN void cb_8to24_Update(const unsigned char *vidBuffer)
{
    // Update 8bit subimage
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cb_8to24__state.textures[CB_8TO24_TEX_BUFFER8]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cb_8to24__state.vidWidth, cb_8to24__state.vidHeight, GL_ALPHA, GL_UNSIGNED_BYTE, vidBuffer);

    // Render 8bit src to 32bit dst
    glBindFramebuffer(GL_FRAMEBUFFER, cb_8to24__state.frameBuffer);

    glViewport(0, 0, cb_8to24__state.vidWidth, cb_8to24__state.vidHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(cb_8to24__state.convertProgram);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Render 32bit src to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(cb_8to24__state.vpX, cb_8to24__state.vpY, cb_8to24__state.vpWidth, cb_8to24__state.vpHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, cb_8to24__state.textures[CB_8TO24_TEX_BUFFER32]);
    glUseProgram(cb_8to24__state.upscaleProgram);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

CB_8TO24_EXTERN void cb_8to24_SetPalette(const unsigned char *palette)
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cb_8to24__state.textures[CB_8TO24_TEX_PALETTE]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGB, GL_UNSIGNED_BYTE, palette);
}

#endif
