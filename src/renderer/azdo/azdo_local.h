// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Local AZDO renderer header file

#ifndef AZDO_LOCAL_H
#define AZDO_LOCAL_H

#include "../../ac130.h"
#ifdef WIN32	// enable static linking on win32
	#define GLEW_STATIC
#endif // WIN32
#include <GL/glew.h>
#define NO_SDL_GLEXT	// GLEW takes care of extensions
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>

/// \file gl14_local.h
/// \brief Private interfaces to all AZDO renderer modules.

/// \addtogroup priv_azdo Private AZDO renderer interface
/// @{

/// Control OpenGL debugging (callback and annotation). Defaults to debug builds.
#ifndef OPENGL_DEBUG
	#ifndef NDEBUG
		#define OPENGL_DEBUG	1
	#else
		#define OPENGL_DEBUG	0
	#endif
#endif

#if OPENGL_DEBUG
    /// Short-hand for OpenGL call grouping: begin event.
	#define OPENGL_EVENT_BEGIN(id, name)									\
		if (GLEW_KHR_debug)													\
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, -1, name)
    /// Short-hand for OpenGL call grouping: end event.
	#define OPENGL_EVENT_END()												\
		if (GLEW_KHR_debug)													\
			glPopDebugGroup()
    /// Short-hand for one-off OpenGL event string marker.
	#define OPENGL_EVENT(id, str)											\
		if (GLEW_KHR_debug)													\
			glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION,				\
				GL_DEBUG_TYPE_OTHER, id, GL_DEBUG_SEVERITY_LOW, -1, str)
#else
    #define OPENGL_EVENT_BEGIN(id, name)
    #define OPENGL_EVENT_END()
    #define OPENGL_EVENT(id, str)
#endif // OPENGL_DEBUG

/// @}

#endif // AZDO_LOCAL_H
