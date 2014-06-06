// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

#ifndef GL14_H
#define GL14_H

/// \file gl14.h
/// \brief Public interface to the OpenGL 1.4 renderer.
/// \addtogroup gl14 OpenGL 1.4 renderer
/// @{

bool gl14_init(uint *vcounter, uint *tcounter,
					uint *dpcounter, uint *cpcounter);
void gl14_shutdown(void);
void gl14_set_heightmap();
void gl14_start_scene(int time, ac_viewpoint_t *vp);
void gl14_start_fx(void);
void gl14_finish_fx(void);
void gl14_draw_fx(ac_vec4_t pos, float scale, float alpha, float angle);
void gl14_draw_tracer(ac_vec4_t pos, ac_vec4_t dir, float scale);
void gl14_start_footmobiles(void);
void gl14_finish_footmobiles(void);
void gl14_draw_squad(ac_footmobile_t *squad, size_t troops);
void gl14_draw_string(char *str, float ox, float oy, float scale);
void gl14_draw_lines(float pts[][2], uint num_pts, float width);
void gl14_finish_3D(void);
void gl14_finish_2D(void);
void gl14_composite(float negative, float contrast);

/// @}

#endif // GL14_H
