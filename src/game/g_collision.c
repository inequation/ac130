// AC-130 shooter
// Written by Leszek Godlewski <github@inequation.org>

// Collision detection

#include "g_local.h"

static inline float g_trace_through_AABB(ac_vec4_t p1, ac_vec4_t p2,
	ac_vec4_t bounds[2]) {
	float d1, d2, f;
	float enterFrac = -1.f, leaveFrac = 1.f;
	bool startOut = false;
	int i;
	if (!bounds)
		return 1.f;
	for (i = 0; i < 6; i++) {
		if (i < 3) {
			d1 = -p1.f[i] + bounds[0].f[i];
			d2 = -p2.f[i] + bounds[0].f[i];
		} else {
			d1 = p1.f[i - 3] - bounds[1].f[i - 3];
			d2 = p2.f[i - 3] - bounds[1].f[i - 3];
		}
		if (d1 > 0)
            startOut = true;
		// if completely in front of face, no intersection with the entire AABB
		if (d1 > 0 && (d2 >= 0.f || d2 >= d1))
			return 1.f;
		// if it doesn't cross the plane, the plane isn't relevent
		if (d1 <= 0 && d2 <= 0)
			continue;
		// crosses face
		if (d1 > d2) {	// enter
			f = d1 / (d1 - d2);
			if (f < 0)
				f = 0;
			if (f > enterFrac)
				enterFrac = f;
		} else {	// leave
			f = d1 / (d1 - d2);
			if (f > 1)
				f = 1;
			if (f < leaveFrac)
				leaveFrac = f;
		}
	}
	if (!startOut)
        return 0.f;
	if (enterFrac < leaveFrac)
		return (enterFrac > 0 ? enterFrac : 0.f);
	return 1.f;
}

static inline float g_trace_through_bldg(ac_vec4_t p1, ac_vec4_t p2,
	ac_bldg_t *b) {
	// OK, let's do this the easy way - transform the ray into the building's
	// own object space and just treat it like a bounding box
	float c, s;
	ac_vec4_t x, z, l1, l2, bounds[2];
	c = cosf(b->ang);
	s = sinf(b->ang);
	// these vectors make up the first and third rows of the inverse rotation
	// matrix
	x = ac_vec_set(c,
					0.f,
					-s,
					0.f);
	z = ac_vec_set(s,
					0.f,
					c,
					0.f);
	// now, do the actual transformation; first, make the translation relative
	p1 = ac_vec_sub(p1, b->pos);
	p2 = ac_vec_sub(p2, b->pos);
	// then undo rotation
	l1 = ac_vec_set(ac_vec_dot(p1, x),
					p1.f[1],
					ac_vec_dot(p1, z),
					0.f);
	l2 = ac_vec_set(ac_vec_dot(p2, x),
					p2.f[1],
					ac_vec_dot(p2, z),
					0.f);
	// calculate the bounding box
	// cheat on the Y axis for slanted roof buildings
	bounds[0] = ac_vec_set(-0.5 * b->Xscale,
							b->slantedRoof ? -1.2 : -1.f,
							-0.5 * b->Zscale,
							0.f);
	bounds[1] = ac_vec_mulf(bounds[0], -1.f);
	// whew, that's about it!
	return g_trace_through_AABB(l1, l2, bounds);
}

static float g_collide_bldgs(ac_vec4_t p1, ac_vec4_t p2, ac_prop_t *node,
	float curFrac) {
	int i;
	float frac;
	frac = g_trace_through_AABB(p1, p2, node->bounds);
	if (node && node->bldgs) {
		for (i = 0; i < BLDGS_PER_FIELD; i++) {
			if ((frac = g_trace_through_bldg(p1, p2, node->bldgs + i))
				< curFrac) {
                printf("LOL %f\n", frac);
				curFrac = frac;
            }
		}
	} else if (frac >= curFrac)
		// if we haven't hit our AABB or we hit it further than the closest hit
		// so far, we can't have anything of interest left
		return 1.f;
	if (!node)
		return curFrac;
	for (i = 0; i < 4; i++) {
		if (!node->child[i])
			continue;
		if ((frac = g_collide_bldgs(p1, p2, node->child[i], curFrac))
			< curFrac)
			curFrac = frac;
	}
	return curFrac;
}

static ac_vec4_t g_collide_terrain(ac_vec4_t p1, ac_vec4_t p2) {
	ac_vec4_t half = ac_vec_setall(0.5);
	ac_vec4_t v = ac_vec_sub(p2, p1);
	ac_vec4_t p;
	float h;
	int i;
	// bisect for at most 4 steps or until a solution within 10cm is found
	for (i = 0; i < 4; i++) {
		v = ac_vec_mul(v, half);
		p = ac_vec_add(p1, v);
		h = g_sample_height(p.f[0], p.f[2]);
		if (fabs(p.f[1] - h) < 0.1)
			break;
		if (p.f[1] < h) {
			p2 = ac_vec_add(p1, v);

		} else
			p1 = ac_vec_add(p1, v);
	}
	p.f[1] = h;
	return p;
}

ac_vec4_t g_collide(ac_vec4_t p1, ac_vec4_t p2) {
	float frac = g_collide_bldgs(p1, p2, gen_proptree, 1.f);
	// clip the trace to the terrain first
	p2 = g_collide_terrain(p1, p2);
	if (frac < 1.f) {
		// TODO
	}
	return p2;
}
