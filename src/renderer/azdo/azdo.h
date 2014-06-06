// AC-130 shooter
// Written by Leszek Godlewski <github@inequation.org>

#ifndef AZDO_H
#define AZDO_H

/// \file gl14.h
/// \brief Public interface to the AZDO (OpenGL 4.x) renderer.
/// \addtogroup azdo AZDO (OpenGL 4.x) renderer
/// @{

bool azdo_init(uint *vcounter, uint *tcounter,
					uint *dpcounter, uint *cpcounter);
void azdo_shutdown(void);
void azdo_set_heightmap();
void azdo_start_scene(int time, ac_viewpoint_t *vp);
void azdo_start_fx(void);
void azdo_finish_fx(void);
void azdo_draw_fx(ac_vec4_t pos, float scale, float alpha, float angle);
void azdo_draw_tracer(ac_vec4_t pos, ac_vec4_t dir, float scale);
void azdo_start_footmobiles(void);
void azdo_finish_footmobiles(void);
void azdo_draw_squad(ac_footmobile_t *squad, size_t troops);
void azdo_draw_string(char *str, float ox, float oy, float scale);
void azdo_draw_lines(float pts[][2], uint num_pts, float width);
void azdo_finish_3D(void);
void azdo_finish_2D(void);
void azdo_composite(float negative, float contrast);

/// @}

#endif // AZDO_H
