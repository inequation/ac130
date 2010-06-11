// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Local game logic header file

#ifndef G_LOCAL_H
#define G_LOCAL_H

#include "../ac130.h"

/// \file g_local.h
/// \brief Private interfaces to all game logic modules.

/// \addtogroup priv_g Private game logic interface
/// @{

typedef enum {
	WP_NONE,
	/// M61 Vulcan minigun bullet
	WP_M61,
	/// L/60 Bofors cannon round
	WP_L60,
	/// M102 howitzer round
	WP_M102,
	/// M61 tracer round; not really a separate weapon, but we need to
	/// distinguish between normal and tracer rounds
	WP_M61_TRACER
} weap_t;

typedef struct {
	weap_t	weap;
	ac_vec4_t	pos;
	ac_vec4_t	vel;
} projectile_t;

typedef struct {
	ac_vec4_t	pos;
	ac_vec4_t	vel;
	float		scale;
	float		life;
	float		alpha;
	float		angle;
	weap_t		weap;
} particle_t;

/// Real rate of fire: 6000 rounds per minute
#define WEAP_FIREDELAY_M61	0.01
/// Real rate of fire: 120 rounds per minute
#define WEAP_FIREDELAY_L60	0.5
/// Real rate of fire: 10 rounds per minute
#define WEAP_FIREDELAY_M102	6
/// Real muzzle velocity: 1050m/s
#define WEAP_MUZZVEL_M61	350//1050
/// Real muzzle velocity: 881m/s
#define WEAP_MUZZVEL_L60	300//881
/// Real muzzle velocity: 494m/s
#define WEAP_MUZZVEL_M102	160//494

/// Amount of time the camera will shake after a M102 shot, in seconds.
#define SHAKE_TIME			0.45
/// Amount of time it takes the display to switch to the negative, in seconds.
#define NEGATIVE_TIME		0.25
/// Amount of time the contrast enhancement effect lasts, in seconds.
#define EXPLOSION_TIME		2.0

// main game module
float g_sample_height(float x, float y);

// collision detection module
/// \brief Performs a ray trace from \e p1 to \e p2.
/// \return		the point hit by the trace
ac_vec4_t g_collide(ac_vec4_t p1, ac_vec4_t p2);

/// @}

#endif // G_LOCAL_H
