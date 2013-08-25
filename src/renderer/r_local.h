// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Local renderer header file

#ifndef R_LOCAL_H
#define R_LOCAL_H

#include "../ac130.h"
#ifdef WIN32	// enable static linking on win32
	#define GLEW_STATIC
#endif // WIN32
#include <GL/glew.h>
#define NO_SDL_GLEXT	// GLEW takes care of extensions
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>

/// \file r_local.h
/// \brief Private interfaces to all renderer modules.

/// \addtogroup priv_r Private renderer interface
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
cullResult_t r_cull_bbox(ac_vec4_t bounds[2]);
/// Performs frustum culling on the given sphere.
/// \return			true if sphere outside the view frustum, false if inside or
///					intersecting
bool r_cull_sphere(ac_vec4_t p, float radius);
// performance counters
extern uint	*r_tri_counter;				///< triangle counter
extern uint	*r_vert_counter;			///< vertex counter
extern uint	*r_visible_patch_counter;	///< visible terrain patches counter
extern uint	*r_culled_patch_counter;	///< culled terrain patches counter
// camera position
extern ac_vec4_t	r_viewpoint;		///< camera position

// terrain rendering engine
/// Creates all terrain resources.
void r_create_terrain(void);
/// Draws the terrain.
void r_draw_terrain(void);
/// Frees terrain resources.
void r_destroy_terrain(void);

// prop drawing engine
/// Creates all prop resources.
void r_create_props(void);
/// Draws all props.
void r_draw_props(void);
/// Frees prop resources.
void r_destroy_props(void);

// FX engine
/// Creates all special effects resources.
void r_create_fx(void);
/// Frees all special effects resources.
void r_destroy_fx(void);

// 2D drawing module
/// Creates font resources.
void r_create_font(void);
/// Frees font resources.
void r_destroy_font(void);

// Footmobile module
/// Creates footmobile resources.
void r_create_footmobile(void);
/// Frees footmobile resources.
void r_destroy_footmobile(void);

// shader module
/// Creates, compiles and links shaders.
/// \return			true on successful compilation and linking, false otherwise
bool r_create_shaders(void);
/// Frees all shader resources.
void r_destroy_shaders(void);
// programs
extern uint	r_ter_prog;			///< terrain rendering program
extern uint	r_prop_prog;		///< prop rendering program
extern uint	r_sprite_prog;		///< sprite program
extern uint	r_fmb_prog;			///< footmobile program
extern uint	r_font_prog;		///< font rendering program
extern uint	r_comp_prog;		///< compositing program
// uniform variables
extern int	r_ter_patch_params;	///< per-patch terrain parameters
extern int	r_ter_height_samples;	///< height samples table
extern int	r_comp_frames;		///< frame texture indices
extern int	r_comp_neg;			///< colour inversion coefficient
extern int	r_comp_contrast;	///< contrast enhancement coefficient

/// @}

#endif // R_LOCAL_H
