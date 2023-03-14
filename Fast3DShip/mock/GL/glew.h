#pragma once

#include <stddef.h>

#if defined(_WIN32)
#include <Windows.h>
#include <gl/GL.h>
#endif

#if !defined(_PTRDIFF_T_DEFINED) && !defined(_PTRDIFF_T_) && !defined(__MINGW64__)
#ifdef _WIN64
typedef __int64 ptrdiff_t;
#else
typedef _W64 int ptrdiff_t;
#endif
#define _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_
#endif

#ifdef APIENTRY
#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif
#ifndef GLEWAPIENTRY
#define GLEWAPIENTRY APIENTRY
#endif
#else
#define GLEW_APIENTRY_DEFINED
#if defined(__MINGW32__) || defined(__CYGWIN__) || (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || \
    defined(__BORLANDC__)
#define APIENTRY __stdcall
#ifndef GLAPIENTRY
#define GLAPIENTRY __stdcall
#endif
#ifndef GLEWAPIENTRY
#define GLEWAPIENTRY __stdcall
#endif
#else
#define APIENTRY
#endif
#endif

#if defined(_WIN32)
#ifndef GLAPI
#if defined(__MINGW32__) || defined(__CYGWIN__)
#define GLAPI extern
#else
#define GLAPI WINGDIAPI
#endif
#endif
#endif

#ifndef GLAPI
#define GLAPI extern
#endif

#ifndef GLEWAPIENTRY
#define GLEWAPIENTRY
#endif

typedef ptrdiff_t GLsizeiptr;

typedef int GLint;
typedef unsigned char GLubyte;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLsizei;
typedef char GLchar;
typedef float GLfloat;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef float GLclampf;

typedef void(GLAPIENTRY* PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void(GLAPIENTRY* PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void(GLAPIENTRY* PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);
typedef void(GLAPIENTRY* PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void(GLAPIENTRY* PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void(GLAPIENTRY* PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef GLuint(GLAPIENTRY* PFNGLCREATEPROGRAMPROC)(void);
typedef GLuint(GLAPIENTRY* PFNGLCREATESHADERPROC)(GLenum type);
typedef void(GLAPIENTRY* PFNGLDELETESHADERPROC)(GLuint shader);
typedef void(GLAPIENTRY* PFNGLDETACHSHADERPROC)(GLuint program, GLuint shader);
typedef void(GLAPIENTRY* PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void(GLAPIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef GLint(GLAPIENTRY* PFNGLGETATTRIBLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void(GLAPIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void(GLAPIENTRY* PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* param);
typedef GLint(GLAPIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void(GLAPIENTRY* PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void(GLAPIENTRY* PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string,
                                                const GLint* length);
typedef void(GLAPIENTRY* PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void(GLAPIENTRY* PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void(GLAPIENTRY* PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void(GLAPIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized,
                                                       GLsizei stride, const void* pointer);
typedef void(GLAPIENTRY* PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void(GLAPIENTRY* PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
typedef void(GLAPIENTRY* PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget,
                                                           GLuint renderbuffer);
typedef void(GLAPIENTRY* PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget,
                                                        GLuint texture, GLint level);
typedef void(GLAPIENTRY* PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint* framebuffers);
typedef void(GLAPIENTRY* PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint* renderbuffers);
typedef void(GLAPIENTRY* PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width,
                                                       GLsizei height);
typedef void(GLAPIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void(GLAPIENTRY* PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint* renderbuffers);
typedef void(GLAPIENTRY* PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void(GLAPIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)(GLenum target, GLsizei samples, GLenum internalformat,
                                                                  GLsizei width, GLsizei height);
typedef void(GLAPIENTRY* PFNGLBLITFRAMEBUFFERPROC)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0,
                                                   GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                                                   GLenum filter);
typedef void(GLAPIENTRY* PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint* framebuffers);

#if defined(_WIN32)
#include <Windows.h>
#include <gl/GL.h>
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
#endif

extern PFNGLACTIVETEXTUREPROC __glewActiveTexture;
extern PFNGLATTACHSHADERPROC __glewAttachShader;
extern PFNGLBINDBUFFERPROC __glewBindBuffer;
extern PFNGLBINDFRAMEBUFFERPROC __glewBindFramebuffer;
extern PFNGLBINDRENDERBUFFERPROC __glewBindRenderbuffer;
extern PFNGLBUFFERDATAPROC __glewBufferData;
extern PFNGLCOMPILESHADERPROC __glewCompileShader;
extern PFNGLCREATEPROGRAMPROC __glewCreateProgram;
extern PFNGLCREATESHADERPROC __glewCreateShader;
extern PFNGLDELETEBUFFERSPROC __glewDeleteBuffers;
extern PFNGLDELETESHADERPROC __glewDeleteShader;
extern PFNGLDETACHSHADERPROC __glewDetachShader;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC __glewDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC __glewEnableVertexAttribArray;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC __glewFramebufferRenderbuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC __glewFramebufferTexture2D;
extern PFNGLGENBUFFERSPROC __glewGenBuffers;
extern PFNGLGENFRAMEBUFFERSPROC __glewGenFramebuffers;
extern PFNGLGENRENDERBUFFERSPROC __glewGenRenderbuffers;
extern PFNGLGETATTRIBLOCATIONPROC __glewGetAttribLocation;
extern PFNGLGETSHADERINFOLOGPROC __glewGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC __glewGetShaderiv;
extern PFNGLGETUNIFORMLOCATIONPROC __glewGetUniformLocation;
extern PFNGLLINKPROGRAMPROC __glewLinkProgram;
extern PFNGLRENDERBUFFERSTORAGEPROC __glewRenderbufferStorage;
extern PFNGLSHADERSOURCEPROC __glewShaderSource;
extern PFNGLUNIFORM1IPROC __glewUniform1i;
extern PFNGLUNIFORM2FPROC __glewUniform2f;
extern PFNGLUSEPROGRAMPROC __glewUseProgram;
extern PFNGLVERTEXATTRIBPOINTERPROC __glewVertexAttribPointer;
extern PFNGLDELETERENDERBUFFERSPROC __glewDeleteRenderbuffers;
extern PFNGLUNIFORM1FPROC __glewUniform1f;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC __glewRenderbufferStorageMultisample;
extern PFNGLBLITFRAMEBUFFERPROC __glewBlitFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC __glewDeleteFramebuffers;

#if defined(_WIN32)
extern PFNWGLSWAPINTERVALEXTPROC __wglewSwapIntervalEXT;
#endif

#define glBindBuffer GLEW_GET_FUN(__glewBindBuffer)
#define glBufferData GLEW_GET_FUN(__glewBufferData)
#define glDeleteBuffers GLEW_GET_FUN(__glewDeleteBuffers)
#define glGenBuffers GLEW_GET_FUN(__glewGenBuffers)
#define glAttachShader GLEW_GET_FUN(__glewAttachShader)
#define glCompileShader GLEW_GET_FUN(__glewCompileShader)
#define glCreateProgram GLEW_GET_FUN(__glewCreateProgram)
#define glCreateShader GLEW_GET_FUN(__glewCreateShader)
#define glDeleteShader GLEW_GET_FUN(__glewDeleteShader)
#define glDetachShader GLEW_GET_FUN(__glewDetachShader)
#define glDisableVertexAttribArray GLEW_GET_FUN(__glewDisableVertexAttribArray)
#define glEnableVertexAttribArray GLEW_GET_FUN(__glewEnableVertexAttribArray)
#define glGetAttribLocation GLEW_GET_FUN(__glewGetAttribLocation)
#define glGetShaderInfoLog GLEW_GET_FUN(__glewGetShaderInfoLog)
#define glGetShaderiv GLEW_GET_FUN(__glewGetShaderiv)
#define glGetUniformLocation GLEW_GET_FUN(__glewGetUniformLocation)
#define glLinkProgram GLEW_GET_FUN(__glewLinkProgram)
#define glShaderSource GLEW_GET_FUN(__glewShaderSource)
#define glUniform1i GLEW_GET_FUN(__glewUniform1i)
#define glUniform2f GLEW_GET_FUN(__glewUniform2f)
#define glUseProgram GLEW_GET_FUN(__glewUseProgram)
#define glVertexAttribPointer GLEW_GET_FUN(__glewVertexAttribPointer)
#define glBindFramebuffer GLEW_GET_FUN(__glewBindFramebuffer)
#define glBindRenderbuffer GLEW_GET_FUN(__glewBindRenderbuffer)
#define glFramebufferRenderbuffer GLEW_GET_FUN(__glewFramebufferRenderbuffer)
#define glFramebufferTexture2D GLEW_GET_FUN(__glewFramebufferTexture2D)
#define glGenFramebuffers GLEW_GET_FUN(__glewGenFramebuffers)
#define glGenRenderbuffers GLEW_GET_FUN(__glewGenRenderbuffers)
#define glRenderbufferStorage GLEW_GET_FUN(__glewRenderbufferStorage)
#define glActiveTexture GLEW_GET_FUN(__glewActiveTexture)
#define glDeleteRenderbuffers GLEW_GET_FUN(__glewDeleteRenderbuffers)
#define glUniform1f GLEW_GET_FUN(__glewUniform1f)
#define glRenderbufferStorageMultisample GLEW_GET_FUN(__glewRenderbufferStorageMultisample)
#define glBlitFramebuffer GLEW_GET_FUN(__glewBlitFramebuffer)
#define glDeleteFramebuffers GLEW_GET_FUN(__glewDeleteFramebuffers)

#define GL_FALSE 0
#define GL_TRUE 1

#define GL_LEQUAL 0x0203
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_TEXTURE_2D 0x0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_FLOAT 0x1406
#define GL_RGBA 0x1908
#define GL_VERSION 0x1F02
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_MIRRORED_REPEAT 0x8370
#define GL_TEXTURE0 0x84C0
#define GL_DEPTH_STENCIL 0x84F9
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_DEPTH_CLAMP 0x864F
#define GL_MIRROR_CLAMP_TO_EDGE 0x8743
#define GL_ARRAY_BUFFER 0x8892
#define GL_STREAM_DRAW 0x88E0
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_COMPILE_STATUS 0x8B81
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41

#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005

#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000

#if defined(_WIN32)
#define wglSwapIntervalEXT WGLEW_GET_FUN(__wglewSwapIntervalEXT)

#ifndef WGLEW_GET_FUN
#define WGLEW_GET_FUN(x) x
#endif
#endif

#ifndef GLEW_GET_FUN
#define GLEW_GET_FUN(x) x
#endif

void glewInit(void);
