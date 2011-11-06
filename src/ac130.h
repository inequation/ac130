// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Main header file

#ifndef AC130_H
#define AC130_H

// some includes shared by all the modules
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <SDL/SDL.h>
#include <assert.h>

// our own math module
#include "ac_math.h"

/// \file ac130.h
/// \brief Public interfaces to all modules.

// =========================================================
/// \addtogroup pub_types Public type definitions
// =========================================================

/// @{

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;

/// Tree prop structure.
typedef struct {
	ac_vec4_t	pos;			///< position at which to spawn the tree
	float		ang;			///< orientation (rotation around the Y axis)
	float		XZscale;		///< horizontal scale factor of the tree
	float		Yscale;			///< vertical scale factor of the tree
} ac_tree_t;

/// Building prop structure.
typedef struct {
	ac_vec4_t	pos;			///< position of the building's origin
	float		ang;			///< orientation (rotation around the Y axis)
	float		Xscale;			///< horiz. X scale factor of the building
	float		Yscale;			///< horiz. Z scale factor of the building
	float		Zscale;			///< vertical scale factor of the building
	bool		slantedRoof;	///< "architectural" setting - if true, the
								///  building has a slanted roof, otherwise a
								///  flatone
} ac_bldg_t;

/// Prop tree node structure.
typedef struct ac_prop_s {
	ac_vec4_t			bounds[2];	///< 2 points describing the AABB (axis-
									///  aligned bounding box) of the node
	struct ac_prop_s	*child[4];	///< pointers to children nodes
	ac_tree_t			*trees;		///< tree array (NULL if node is a
									///  branch or a building prop leaf)
	ac_bldg_t			*bldgs;		///< building array (NULL if node is a
									///  branch or a tree prop leaf)
} ac_prop_t;

/// Viewpoint definition structure.
typedef struct {
	ac_vec4_t	origin;		///< camera position
	float		angles[2];	///< camera direction defined by yaw and pitch
							///  angles
	float		fov;		///< field of view angle in radians
} ac_viewpoint_t;

/// Geometry vertex.
typedef struct {
	ac_vec4_t	pos;		///< vertex position
	float		st[2];		///< texture coordinates
	int			index;		///< geometry instance index
} ac_vertex_t;

/// Footmobile (ground troop) stance enumeration.
typedef enum {
	STANCE_STAND,			///< standing (waiting or moving)
	STANCE_CROUCH			///< crouching (firing)
} ac_stance_t;

/// Footmobile (ground troop) data structure.
typedef struct {
	ac_vec4_t	pos;
	float		ang;
	ac_stance_t	stance;
	int			health;
} ac_footmobile_t;

/// @}

// =========================================================
/// \addtogroup pub_main Public main module interface
// =========================================================

/// @{

/// game screen width in pixels
extern int m_screen_width;
/// game screen height in pixels
extern int m_screen_height;
/// whether the game is running in full screen mode or not
extern bool m_full_screen;

/// terrain level of detail setting (adjustable by -t <LOD> commandline option)
extern float m_terrain_LOD;
/// whether full-screen anti-aliasing is enabled (defaults to true, -noaa commandline option to disable)
extern bool m_FSAA;
/// whether compatbilitiy mode shaders are enabled (defaults to false, -c commandline option to enable)
extern bool m_compatshader;

/// @}

// =========================================================
/// \addtogroup pub_gen Public content generator interface
// =========================================================

/// @{

/// \brief size of terrain height map
/// (in pixels; 1 pixel translates to 1 square metre in game world)
#define HEIGHTMAP_SIZE		1024
/// \brief height amplitude in metres
#define HEIGHT				50.f
/// \brief height scaling factor
/// used when converting from heightmap bytes to game world height
#define HEIGHT_SCALE		(HEIGHT / 255.f)

/// \brief number of vertices in the base of the tree
/// at the highest level of detail
#define TREE_BASE			7
/// \brief number of vertices in a building prop with a flat roof
#define BLDG_FLAT_VERTS		8
/// \brief number of indices in a flat-roofed building prop's triangle strip
#define BLDG_FLAT_INDICES	16
/// \brief number of vertices in a building prop with a slanted roof
#define BLDG_SLNT_VERTS		10
/// \brief number of indices in a slanted-roofed building prop's triangle strip
#define BLDG_SLNT_INDICES	28
/// \brief dimension of the prop texture (both width and height)
#define PROP_TEXTURE_SIZE	64

/// \brief bit shift to apply when operating on the prop map;
/// 2^4 = 16, which means that 1 square of the prop map covers a 16*16 square of
/// the height map
#define PROPMAP_SHIFT		4
/// \brief dimension of the prop map (both width and height)
#define PROPMAP_SIZE		(HEIGHTMAP_SIZE >> PROPMAP_SHIFT)
/// \brief fraction of the entire terrain's surface area to be covered by trees
#define TREE_COVERAGE		0.6
/// \brief fraction of the entire terrain's surf. area to be used by buildings
#define BLDG_COVERAGE		0.08
/// \brief number of trees to plant per prop map square
#define TREES_PER_FIELD		25
/// \brief number of buildings to plant per prop map square
#define BLDGS_PER_FIELD		1
/// \brief maximum number of props (either trees or buildings) per prop map square
#define PROPS_PER_FIELD		(TREES_PER_FIELD > BLDGS_PER_FIELD				\
								? TREES_PER_FIELD : BLDGS_PER_FIELD)

/// \brief maximum number of trees in the entire game world
#define MAX_NUM_TREES		(TREES_PER_FIELD								\
								* PROPMAP_SIZE * PROPMAP_SIZE * TREE_COVERAGE)
/// \brief maximum number of buildings in the entire game world
#define MAX_NUM_BLDGS		(BLDGS_PER_FIELD								\
								* PROPMAP_SIZE * PROPMAP_SIZE * BLDG_COVERAGE)

/// \brief dimension of the special effects texture (both width and height)
#define FX_TEXTURE_SIZE		256

/// \brief heightmap byte array
extern uchar				gen_heightmap[];
/// \brief root of the prop tree
extern ac_prop_t			*gen_proptree;

/// \brief Generates the terrain heightmap.
/// \note				The heightmap is stored in stack memory, therefore it
///						should not be freed.
/// \return				constant pointer to the heightmap
/// \param seed			random number seed; ensures identical random number
///						sequence each run
void gen_terrain(int seed);

/// \brief Generates props (trees, buildings) resources.
/// \param texture		texture byte array
/// \param verts		vertex array
/// \param indices		index array
void gen_props(uchar *texture, ac_vertex_t *verts, uchar *indices);


/// \brief Generates special effects resources.
/// \param texture		texture byte array
/// \param verts		vertex array
/// \param indices		index array
void gen_fx(uchar *texture, ac_vertex_t *verts, uchar *indices);

/// \brief Generates prop (tree and buildings) lists.
/// \note				Both trees and bldgs must be preallocated by the caller.
/// \param numTrees		pointer to where to store the tree count
/// \param trees		the array to which to write the prop placement
///						information
/// \param numBldgs		pointer to where to store the building count
/// \param bldgs		the array to which to write the prop placement
///						information
void gen_proplists(int *numTrees, ac_tree_t *trees,
					int *numBldgs, ac_bldg_t *bldgs);

/// \brief Frees the given prop tree node and all of its children.
/// \param n			pointer to the node of the tree to free (pass NULL to
///						free the entire tree)
void gen_free_proptree(ac_prop_t *n);

/// @}

// =========================================================
/// \addtogroup pub_r Public renderer interface
// =========================================================

/// @{

/// \brief Initializes the renderer.
/// \param vcounter		vertex counter address (for performance measurement)
/// \param tcounter		triangle counter address (for performance measurement)
/// \param dpcounter	displayed terrain patch counter address
/// \param cpcounter	culled terrain patch counter address
/// \return				true on success
bool r_init(uint *vcounter, uint *tcounter,
					uint *dpcounter, uint *cpcounter);

/// \brief Shuts the renderer down.
void r_shutdown(void);

/// \brief Sets new terrain heightmap.
void r_set_heightmap();

/// \brief Starts the rendering of a new frame. Also sets the point of view.
/// \note				Must be called *before* \ref r_finish_3D
void r_start_scene(int time, ac_viewpoint_t *vp);

/// \brief Starts the FX rendering stage.
/// Makes the necessary state changes, etc.
/// \sa r_finish_fx
void r_start_fx(void);

/// \brief Finishes the FX rendering stage.
/// Makes the necessary state changes, etc.
/// \sa r_start_fx
void r_finish_fx(void);

/// \brief Draws a smoke particle.
/// Draws a smoke particle at the given position with the given scale and alpha.
/// \param pos			position of the smoke particle
/// \param scale		scale of the particle
/// \param alpha		alpha (transparency) value of the particle
/// \param angle		angle by which to rotate the particle
void r_draw_fx(ac_vec4_t pos, float scale, float alpha, float angle);

/// \brief Draws a bullet tracer at the given position in the given direction.
/// \note				Must be called *after* \ref r_start_fx and
///						*before* \ref r_finish_fx
/// \param pos			position of the tracer
/// \param dir			tracer's direction
/// \param scale		scale of the tracer (length in metres, width in pixels)
void r_draw_tracer(ac_vec4_t pos, ac_vec4_t dir, float scale);

/// \brief Starts the footmobile rendering stage.
/// Makes the necessary state changes, etc.
/// \sa r_finish_footmobiles
void r_start_footmobiles(void);

/// \brief Starts the footmobile rendering stage.
/// Makes the necessary state changes, etc.
/// \sa r_start_footmobiles
void r_finish_footmobiles(void);

/// \brief Draws a footmobile squad of the given size.
/// \param squad		pointer to a footmobile array
/// \param troops		number of soldiers in the squad
void r_draw_squad(ac_footmobile_t *squad, size_t troops);

/// \brief Draws a string at given normalized coordinates in given scale.
/// Coordinates must fall in the [0..1] range. The text will be top-left-
/// aligned. Newlines ('\\n' characters) will be respected, but only the
/// printable ASCII characters in the 32-91 code range will be drawn (others
/// will be replaced by spaces).
/// \note				Negative coordinates will invert the alignment on the
///						corresponding axis; e.g. passing (-1, -1) will cause the
///						text to be drawn from the bottom-right corner, growing
///						to the top and left
/// \param str			pointer to a null-terminated string
/// \param ox			X coordinate of the text origin
/// \param oy			Y coordinate of the text origin
/// \param scale		scale of the text
void r_draw_string(char *str, float ox, float oy, float scale);

/// \brief Draws a set of line segments.
/// Coordinates must be normalized (in the [0..1] range).
/// \param pts			array of 2-element float arrays that contain the point
///						coordinates
/// \param num_pts		number of points in the array
/// \param width		desired width of the line segments in pixels
void r_draw_lines(float pts[][2], uint num_pts, float width);

/// \brief Finishes the 3D rendering stage.
/// Flushes the scene to the render target and switches to the 2D (HUD) stage.
/// \note				Must be called *after* \ref r_start_scene and
///						*before* \ref r_finish_2D
void r_finish_3D(void);

/// \brief Finishes the 2D rendering stage.
/// Flushes the 2D (HUD) elements to the render target.
/// \note				Must be called *after* \ref r_finish_3D and
///						*before* \ref r_composite
void r_finish_2D(void);

/// \brief Combines the 3D and 2D parts of the scene.
/// Also runs post-processing effects and outputs the result frame to screen.
/// \param negative		fraction of display negative influence (for smooth
///						positive-negative transitions)
/// \param contrast		fraction of contrast enhancement (for more prominent
///						M102 explosions)
void r_composite(float negative, float contrast);

/// @}

// =========================================================
/// \addtogroup pub_g Game logic interface
// =========================================================

/// @{

/// Bitflags representing the states of different buttons and keys.
typedef enum {
/// player is holding the colour inversion toggle key
	INPUT_NEGATIVE		= 0x01,
/// player is holding the left mouse button
	INPUT_MOUSE_LEFT	= 0x02,
/// player is holding the right mouse button
	INPUT_MOUSE_RIGHT	= 0x04,
/// player is holding the 1 key
	INPUT_1				= 0x08,
/// player is holding the 2 key
	INPUT_2				= 0x10,
/// player is holding the 3 key
	INPUT_3				= 0x20,
/// player is holding the pause key
	INPUT_PAUSE			= 0x40
} ac_input_flags_t;

/// Internal player input data structure.
typedef struct {
	ac_input_flags_t	flags;			///< button and key state bitfield
	short				deltaX, deltaY;	///< mouse motion deltas
} ac_input_t;

/// \brief Initializes the game logic.
/// \return true on success
bool g_init(void);

/// \brief Shuts the game logic down.
void g_shutdown(void);

/// \brief Advances the game world by one frame.
/// \param ticks		number of ticks (milliseconds) since the start of game
/// \param frameTime	time elapsed since last frame in seconds
/// \param input		current state of player input
void g_frame(int ticks, float frameTime, ac_input_t *input);

/// \brief Updates the game loading screen.
/// \note				Only to be called before \ref g_init
void g_loading_tick(void);

/// @}

#endif // AC130_H
