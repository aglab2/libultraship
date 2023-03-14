#include "GL/glew.h"

PFNGLACTIVETEXTUREPROC __glewActiveTexture = NULL;
PFNGLATTACHSHADERPROC __glewAttachShader = NULL;
PFNGLBINDBUFFERPROC __glewBindBuffer = NULL;
PFNGLBINDFRAMEBUFFERPROC __glewBindFramebuffer = NULL;
PFNGLBINDRENDERBUFFERPROC __glewBindRenderbuffer = NULL;
PFNGLBUFFERDATAPROC __glewBufferData = NULL;
PFNGLCOMPILESHADERPROC __glewCompileShader = NULL;
PFNGLCREATEPROGRAMPROC __glewCreateProgram = NULL;
PFNGLCREATESHADERPROC __glewCreateShader = NULL;
PFNGLDELETEBUFFERSPROC __glewDeleteBuffers = NULL;
PFNGLDELETESHADERPROC __glewDeleteShader = NULL;
PFNGLDETACHSHADERPROC __glewDetachShader = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC __glewDisableVertexAttribArray = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC __glewEnableVertexAttribArray = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC __glewFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC __glewFramebufferTexture2D = NULL;
PFNGLGENBUFFERSPROC __glewGenBuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC __glewGenFramebuffers = NULL;
PFNGLGENRENDERBUFFERSPROC __glewGenRenderbuffers = NULL;
PFNGLGETATTRIBLOCATIONPROC __glewGetAttribLocation = NULL;
PFNGLGETSHADERINFOLOGPROC __glewGetShaderInfoLog = NULL;
PFNGLGETSHADERIVPROC __glewGetShaderiv = NULL;
PFNGLGETUNIFORMLOCATIONPROC __glewGetUniformLocation = NULL;
PFNGLLINKPROGRAMPROC __glewLinkProgram = NULL;
PFNGLRENDERBUFFERSTORAGEPROC __glewRenderbufferStorage = NULL;
PFNGLSHADERSOURCEPROC __glewShaderSource = NULL;
PFNGLUNIFORM1IPROC __glewUniform1i = NULL;
PFNGLUNIFORM2FPROC __glewUniform2f = NULL;
PFNGLUSEPROGRAMPROC __glewUseProgram = NULL;
PFNGLVERTEXATTRIBPOINTERPROC __glewVertexAttribPointer = NULL;
PFNGLDELETERENDERBUFFERSPROC __glewDeleteRenderbuffers = NULL;
PFNGLUNIFORM1FPROC __glewUniform1f = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC __glewRenderbufferStorageMultisample = NULL;
PFNGLBLITFRAMEBUFFERPROC __glewBlitFramebuffer = NULL;
PFNGLDELETEFRAMEBUFFERSPROC __glewDeleteFramebuffers = NULL;

#if defined(_WIN32)
PFNWGLSWAPINTERVALEXTPROC __wglewSwapIntervalEXT = NULL;
#endif

typedef const GLubyte*(GLAPIENTRY* PFNGLGETSTRINGPROC)(GLenum name);

/*
 * Define glewGetProcAddress.
 */
#if defined(GLEW_REGAL)
#define glewGetProcAddress(name) regalGetProcAddress((const GLchar*)name)
#elif defined(GLEW_OSMESA)
#define glewGetProcAddress(name) OSMesaGetProcAddress((const char*)name)
#elif defined(GLEW_EGL)
#define glewGetProcAddress(name) eglGetProcAddress((const char*)name)
#elif defined(_WIN32)
#define glewGetProcAddress(name) wglGetProcAddress((LPCSTR)name)
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
#define glewGetProcAddress(name) NSGLGetProcAddress(name)
#elif defined(__sgi) || defined(__sun) || defined(__HAIKU__)
#define glewGetProcAddress(name) dlGetProcAddress(name)
#elif defined(__ANDROID__)
#define glewGetProcAddress(name) NULL /* TODO */
#elif defined(__native_client__)
#define glewGetProcAddress(name) NULL /* TODO */
#else                                 /* __linux */
#define glewGetProcAddress(name) (*glXGetProcAddressARB)(name)
#endif

extern void wglewInit();
void glewInit(void) {
#if defined(GLEW_EGL)
    PFNEGLGETCURRENTDISPLAYPROC getCurrentDisplay = NULL;
#endif

    // Common functions exports
    // 1.3
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)glewGetProcAddress((const GLubyte*)"glActiveTexture");
    // 1.5
    glBindBuffer = (PFNGLBINDBUFFERPROC)glewGetProcAddress((const GLubyte*)"glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)glewGetProcAddress((const GLubyte*)"glBufferData");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glDeleteBuffers");
    glGenBuffers = (PFNGLGENBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glGenBuffers");
    // 2.0
    glAttachShader = (PFNGLATTACHSHADERPROC)glewGetProcAddress((const GLubyte*)"glAttachShader");
    glCompileShader = (PFNGLCOMPILESHADERPROC)glewGetProcAddress((const GLubyte*)"glCompileShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glCreateProgram");
    glCreateShader = (PFNGLCREATESHADERPROC)glewGetProcAddress((const GLubyte*)"glCreateShader");
    glDeleteShader = (PFNGLDELETESHADERPROC)glewGetProcAddress((const GLubyte*)"glDeleteShader");
    glDetachShader = (PFNGLDETACHSHADERPROC)glewGetProcAddress((const GLubyte*)"glDetachShader");
    glDisableVertexAttribArray =
        (PFNGLDISABLEVERTEXATTRIBARRAYPROC)glewGetProcAddress((const GLubyte*)"glDisableVertexAttribArray");
    glEnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)glewGetProcAddress((const GLubyte*)"glEnableVertexAttribArray");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)glewGetProcAddress((const GLubyte*)"glGetAttribLocation");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glewGetProcAddress((const GLubyte*)"glGetShaderInfoLog");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)glewGetProcAddress((const GLubyte*)"glGetShaderiv");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glewGetProcAddress((const GLubyte*)"glGetUniformLocation");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glLinkProgram");
    glShaderSource = (PFNGLSHADERSOURCEPROC)glewGetProcAddress((const GLubyte*)"glShaderSource");
    glUniform1i = (PFNGLUNIFORM1IPROC)glewGetProcAddress((const GLubyte*)"glUniform1i");
    glUniform2f = (PFNGLUNIFORM2FPROC)glewGetProcAddress((const GLubyte*)"glUniform2f");
    glUseProgram = (PFNGLUSEPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glUseProgram");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribPointer");
    glUniform1f = (PFNGLUNIFORM1FPROC)glewGetProcAddress((const GLubyte*)"glUniform1f");
    // GL_ARB_framebuffer_object
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)glewGetProcAddress((const GLubyte*)"glBindFramebuffer");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)glewGetProcAddress((const GLubyte*)"glBindRenderbuffer");
    glFramebufferRenderbuffer =
        (PFNGLFRAMEBUFFERRENDERBUFFERPROC)glewGetProcAddress((const GLubyte*)"glFramebufferRenderbuffer");
    glFramebufferTexture2D =
        (PFNGLFRAMEBUFFERTEXTURE2DPROC)glewGetProcAddress((const GLubyte*)"glFramebufferTexture2D");
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glGenFramebuffers");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glGenRenderbuffers");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)glewGetProcAddress((const GLubyte*)"glRenderbufferStorage");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glDeleteRenderbuffers");
    glRenderbufferStorageMultisample =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)glewGetProcAddress((const GLubyte*)"glRenderbufferStorageMultisample");
    glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)glewGetProcAddress((const GLubyte*)"glBlitFramebuffer");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glDeleteFramebuffers");

    // OS specific stuff
#if defined(GLEW_EGL)
    getCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC)glewGetProcAddress("eglGetCurrentDisplay");
    return eglewInit(getCurrentDisplay());
#elif defined(GLEW_OSMESA) || defined(__ANDROID__) || defined(__native_client__) || defined(__HAIKU__)
    return r;
#elif defined(_WIN32)
    return wglewInit();
#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX) /* _UNIX */
    return glxewInit();
#else
    return r;
#endif /* _WIN32 */
}

#if defined(_WIN32)
void wglewInit() {
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)glewGetProcAddress((const GLubyte*)"wglSwapIntervalEXT");
}
#endif
