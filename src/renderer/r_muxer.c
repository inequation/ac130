// AC-130 shooter
// Written by Leszek Godlewski <github@inequation.org>

// Renderer backend multiplexer

#include "../ac130.h"
#include "gl14/gl14.h"
#include "azdo/azdo.h"

SDL_Window	*r_screen;

// Pointers
bool (*r_init)(uint *vcounter, uint *tcounter,
					uint *dpcounter, uint *cpcounter);
void (*r_shutdown)(void);
void (*r_set_heightmap)();
void (*r_start_scene)(int time, ac_viewpoint_t *vp);
void (*r_start_fx)(void);
void (*r_finish_fx)(void);
void (*r_draw_fx)(ac_vec4_t pos, float scale, float alpha, float angle);
void (*r_draw_tracer)(ac_vec4_t pos, ac_vec4_t dir, float scale);
void (*r_start_footmobiles)(void);
void (*r_finish_footmobiles)(void);
void (*r_draw_squad)(ac_footmobile_t *squad, size_t troops);
void (*r_draw_string)(char *str, float ox, float oy, float scale);
void (*r_draw_lines)(float pts[][2], uint num_pts, float width);
void (*r_finish_3D)(void);
void (*r_finish_2D)(void);
void (*r_composite)(float negative, float contrast);

void r_setup_gl14() {
	r_init					= gl14_init;
	r_shutdown				= gl14_shutdown;
	r_set_heightmap			= gl14_set_heightmap;
	r_start_scene			= gl14_start_scene;
	r_start_fx				= gl14_start_fx;
	r_finish_fx				= gl14_finish_fx;
	r_draw_fx				= gl14_draw_fx;
	r_draw_tracer			= gl14_draw_tracer;
	r_start_footmobiles		= gl14_start_footmobiles;
	r_finish_footmobiles	= gl14_finish_footmobiles;
	r_draw_squad			= gl14_draw_squad;
	r_draw_string			= gl14_draw_string;
	r_draw_lines			= gl14_draw_lines;
	r_finish_3D				= gl14_finish_3D;
	r_finish_2D				= gl14_finish_2D;
	r_composite				= gl14_composite;
}

void r_setup_azdo() {
	// FIXME: this backend is currently a no-op that always fails to init
	r_init					= azdo_init;
	r_shutdown				= azdo_shutdown;
	/*r_set_heightmap			= azdo_set_heightmap;
	r_start_scene			= azdo_start_scene;
	r_start_fx				= azdo_start_fx;
	r_finish_fx				= azdo_finish_fx;
	r_draw_fx				= azdo_draw_fx;
	r_draw_tracer			= azdo_draw_tracer;
	r_start_footmobiles		= azdo_start_footmobiles;
	r_finish_footmobiles	= azdo_finish_footmobiles;
	r_draw_squad			= azdo_draw_squad;
	r_draw_string			= azdo_draw_string;
	r_draw_lines			= azdo_draw_lines;
	r_finish_3D				= azdo_finish_3D;
	r_finish_2D				= azdo_finish_2D;
	r_composite				= azdo_composite;*/
}
