typedef unsigned int EGLBoolean;
typedef void *EGLDisplay;
#include <EGL/eglplatform.h>
typedef void *EGLConfig;
typedef void *EGLSurface;
typedef void *EGLContext;

#define EGL_TRUE                          1

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface surface);
