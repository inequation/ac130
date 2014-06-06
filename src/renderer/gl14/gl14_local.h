// AC-130 shooter
// Written by Leszek Godlewski <github@inequation.org>

// Local GL 1.4 renderer header file

#ifndef R_LOCAL_H
#define R_LOCAL_H

#include "../../ac130.h"
#ifdef WIN32	// enable static linking on win32
	#define GLEW_STATIC
#endif // WIN32
#include <GL/glew.h>
#define NO_SDL_GLEXT	// GLEW takes care of extensions
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>

/// \file gl14_local.h
/// \brief Private interfaces to all GL 1.4 renderer modules.

/// \addtogroup priv_gl14 Private OpenGL 1.4 renderer interface
/// @{

/// Convenience define for an OpenGL 4x4 matrix.
typedef GLfloat	GLmatrix_t[16];

/// Enumeration that describes the result of the culling operation.
typedef enum {
	CR_OUTSIDE,		///< AABB is outside the view frustum
	CR_INSIDE,		///< AABB is inside the view frustum
	CR_INTERSECT	///< AABB intersects with the view frustum
} cullResult_t;

/// How many past frames are kept for the camera inertia effect.
#define FRAME_TRACE	5
/// Always valid index of the FBO for 2D drawing.
#define FBO_2D		0

/// Single terrain patch side length.
#define TERRAIN_PATCH_SIZE			17
/// Floating point constant version of \a TERRAIN_PATCH_SIZE.
#define TERRAIN_PATCH_SIZE_F		17.f

// main module
/// Performs frustum culling on the given AABB (axis-aligned bounding box).
/// \return			see \ref cullResult_t
cullResult_t gl14_cull_bbox(ac_vec4_t bounds[2]);
/// Performs frustum culling on the given sphere.
/// \return			true if sphere outside the view frustum, false if inside or
///					intersecting
bool gl14_cull_sphere(ac_vec4_t p, float radius);
// performance counters
extern uint	*gl14_tri_counter;				///< triangle counter
extern uint	*gl14_vert_counter;			///< vertex counter
extern uint	*gl14_visible_patch_counter;	///< visible terrain patches counter
extern uint	*gl14_culled_patch_counter;	///< culled terrain patches counter
// camera position
extern ac_vec4_t	gl14_viewpoint;		///< camera position

// terrain rendering engine
/// Creates all terrain resources.
void gl14_create_terrain(void);
/// Draws the terrain.
void gl14_draw_terrain(void);
/// Frees terrain resources.
void gl14_destroy_terrain(void);

// prop drawing engine
/// Creates all prop resources.
void gl14_create_props(void);
/// Draws all props.
void gl14_draw_props(void);
/// Frees prop resources.
void gl14_destroy_props(void);

// FX engine
/// Creates all special effects resources.
void gl14_create_fx(void);
/// Frees all special effects resources.
void gl14_destroy_fx(void);

// 2D drawing module
/// Creates font resources.
void gl14_create_font(void);
/// Frees font resources.
void gl14_destroy_font(void);

// Footmobile module
/// Creates footmobile resources.
void gl14_create_footmobile(void);
/// Frees footmobile resources.
void gl14_destroy_footmobile(void);

// shader module
/// Creates, compiles and links shaders.
/// \return			true on successful compilation and linking, false otherwise
bool gl14_create_shaders(void);
/// Frees all shader resources.
void gl14_destroy_shaders(void);
// programs
extern uint	gl14_ter_prog;			///< terrain rendering program
extern uint	gl14_prop_prog;		///< prop rendering program
extern uint	gl14_sprite_prog;		///< sprite program
extern uint	gl14_fmb_prog;			///< footmobile program
extern uint	gl14_font_prog;		///< font rendering program
extern uint	gl14_comp_prog;		///< compositing program
// uniform variables
extern int	gl14_ter_patch_params;	///< per-patch terrain parameters
extern int	gl14_ter_height_samples;	///< height samples table
extern int	gl14_comp_frames;		///< frame texture indices
extern int	gl14_comp_neg;			///< colour inversion coefficient
extern int	gl14_comp_contrast;	///< contrast enhancement coefficient

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

#endif // R_LOCAL_H
