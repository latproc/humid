/*
 * File:   OpenGLDefinitions.h
 * Author: sylem
 *
 * Created on 30 December 2011, 1:23 AM
 */

#ifndef OPEN_GL_DEFINITIONS_H
#define	OPEN_GL_DEFINITIONS_H

#ifdef __gnu_linux__
    #undef __gl_h_
    #define GL_GLEXT_PROTOTYPES
    #include <GL/gl.h>
    // #include <GL/glu.h>
    #include <GL/glext.h>
    // #include <GL/glx.h>
#elif defined(WIN32)
    #include <stdafx.h>
#elif defined(__APPLE__)
    #include "TargetConditionals.h"

    #if TARGET_OS_IPHONE | TARGET_IPHONE_SIMULATOR // need -DTARGET_OS_IPHONE=1 when building a framework on ios
        // <OpenGLES/gltypes.h>
        // #import <OpenGLES/ES1/g>
        #import <OpenGLES/ES1/gl.h>
    #elif TARGET_OS_MAC
        #include <OpenGL/gl3.h>
        // #include <OpenGL/GLU.h>
        #include <OpenGL/OpenGL.h>
    #endif
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glext.h>
    #include <GL/glx.h>
#endif

#if DRU_IOS
    #define DRU_FACE_POINTER GLushort
    #define DRU_FACE_POINTER_GL_VALUE GL_UNSIGNED_SHORT
#else
    #define DRU_FACE_POINTER uint32_t
    #define DRU_FACE_POINTER_GL_VALUE GL_UNSIGNED_INT
#endif

#endif	/* OPEN_GL_DEFINITIONS_H */
