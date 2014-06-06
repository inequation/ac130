// AC-130 shooter - rendrer module
// Written by Leszek Godlewski <github@inequation.org>

// Procedural content generation module

#include "ac130.h"
#include <assert.h>
#include <stdio.h>

// make sure we don't use the libc rand() in this file!
#define rand()	assert(!"Are you kidding me?!")

uchar			gen_heightmap[HEIGHTMAP_SIZE * HEIGHTMAP_SIZE];
ac_prop_t		*gen_proptree = NULL;

/// Seed for the internal pseudorandom number generator.
static uint		gen_seed = 0;

/// Internal pseudorandom number generator.
int gen_rand(void) {
	gen_seed = (16807LL * (gen_seed + 1)) % 2147483647;
	return gen_seed - 1;
}

/// Perlin noise permutation table.
static int p[] = { 151,160,137,91,90,15,
   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

#define fade(t)			((t) * (t) * (t) * ((t) * ((t) * 6 - 15) + 10))
#define lerp(t, a, b)	((a) + (t) * ((b) - (a)))
static inline float grad(int hash, float x, float y, float z) {
	int h = hash & 15;	// convert low 4 bits of hash code into 12 gradient
	float u = h < 8 ? x : y;	// directions
	float v = h < 4 ? y : (h == 12 || h==14 ? x : z);
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

/// Improved perlin noise generator.
static float gen_perlin(float x, float y, float z) {
	int X, Y, Z;
	int A, AA, AB, B, BA, BB;
	float u, v, w;
	// find unit cube that contains point
	X = (int)floorf(x) & 255;
	Y = (int)floorf(y) & 255;
	Z = (int)floorf(z) & 255;
	// find relative x, y, z of point in cube
	x -= floorf(x);
	y -= floorf(y);
	z -= floorf(z);
	// compute fade curves for each of x, y, z
	u = fade(x);
	v = fade(y);
	w = fade(z);
	// hash coordinates of the 8 cube corners...
	A  = p[X % 256] + Y;
	AA = p[A % 256] + Z;
	AB = p[(A + 1) % 256] + Z;
	B  = p[(X + 1) % 256] + Y;
	BA = p[B % 256] + Z;
	BB = p[(B + 1) % 256]+Z;
	// ...and add blended results from 8 corners of cube
	return lerp(w, lerp(v,  lerp(u, grad(p[ AA    % 256], x  , y  , z   ),
									grad(p[ BA    % 256], x-1, y  , z   )),
							lerp(u, grad(p[ AB    % 256], x  , y-1, z   ),
									grad(p[ BB    % 256], x-1, y-1, z   ))),
					lerp(v, lerp(u, grad(p[(AA+1) % 256], x  , y  , z-1 ),
									grad(p[(BA+1) % 256], x-1, y  , z-1 )),
							lerp(u, grad(p[(AB+1) % 256], x  , y-1, z-1 ),
									grad(p[(BB+1) % 256], x-1, y-1, z-1 ))));
}
#undef fade
#undef lerp

static void gen_cloudmap(char *dst, size_t size) {
	size_t i, x, y, xoff, yoff;
	int pix;
	char *submaps[3];
	char *c;
	float freq;

	freq = 0.015 + 0.000001 * (float)((gen_rand() % 10000) - 5000);

	for (x = 0; x < sizeof(submaps) / sizeof(submaps[0]); x++)
		submaps[x] = malloc(size * size);

	// fill the maps with noise
	for (i = 0, c = dst;
		i < 1 + sizeof(submaps) / sizeof(submaps[0]);
		i++, freq *= 2.0) {
		if (i > 0)
			c = submaps[i - 1];
		xoff = gen_rand() % (size * 2);
		yoff = gen_rand() % (size * 2);
		for (y = 0; y < size; y++) {
			for (x = 0; x < size; x++)
				c[y * size + x] = (char)(127.f
					* gen_perlin(
						(float)(x + xoff) * freq,
						(float)(y + yoff) * freq,
						sqrtf((x + xoff) * (y + yoff)) * freq));
			g_loading_tick();
		}
	}

	// combine maps
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			for (i = 0, c = submaps[i];
			i < sizeof(submaps) / sizeof(submaps[0]);
			i++, c = submaps[i]) {
				pix = (int)(dst[y * size + x])
					+ (int)(c[y * size + x]) / (2 << i);
				if (pix < -128)
					pix = -128;
				else if (pix > 127)
					pix = 127;
				dst[y * size + x] = (char)pix;
			}
		}
		g_loading_tick();
	}

	for (x = 0; x < sizeof(submaps) / sizeof(submaps[0]); x++)
		free(submaps[x]);
}

/// Attempts to load a cached copy of the height map from disk, if available.
static bool gen_load_terrain_cache(int seed) {
	FILE *f;
	char fname[16];
	size_t r;

	snprintf(fname, sizeof(fname), "%X.act", *((uint *)&seed));
	f = fopen(fname, "rb");

	if (!f)
		return false;
	r = fread(gen_heightmap, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, f);
	fclose(f);
	return r == HEIGHTMAP_SIZE;
}

/// Saves a generated height map to a disk cache for future reuse.
static void gen_save_terrain_cache(int seed) {
	FILE *f;
	char fname[16];

	snprintf(fname, sizeof(fname), "%X.act", *((uint *)&seed));
	f = fopen(fname, "wb");

	if (!f)
		return;
	fwrite(gen_heightmap, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, f);
	fclose(f);
}

void gen_terrain(int seed) {
	int x, y, xoff, yoff, pix;
	char *cloudmap = malloc(sizeof(gen_heightmap));
	float freq;

	if (gen_load_terrain_cache(seed))
		return;

	// HACK: this xor is a litle manipulation to keep a pre-bugfix landscape for
	// a particular random seed (the seed used to be initialized after the
	// frequency) while maintaining the randomness of the algorithm
	gen_seed = seed ^ 0xDEADBEEF;
	freq = 0.005 + 0.000001 * (float)((gen_rand() % 6000) - 3000);
	gen_seed = seed;
	xoff = gen_rand() % (HEIGHTMAP_SIZE);
	yoff = gen_rand() % (HEIGHTMAP_SIZE);

	memset(gen_heightmap, 127, sizeof(gen_heightmap));

	gen_cloudmap(cloudmap, HEIGHTMAP_SIZE);

	for (y = 0; y < HEIGHTMAP_SIZE; y++) {
		for (x = 0; x < HEIGHTMAP_SIZE; x++) {
#if 1
			// pass 1 - rough topography
			((char *)gen_heightmap)[y * HEIGHTMAP_SIZE + x] += (char)(127.f
				* gen_perlin(
					(float)(x + xoff) * freq,
					(float)(y + yoff) * freq,
					sqrtf((x + xoff) * (y + yoff)) * freq));
#endif
#if 0
			// pass 2 - detail
			freq *= 1.00002;
			((char *)gen_heightmap)[y * HEIGHTMAP_SIZE + x] += (char)(127.f
				* gen_perlin(
					(float)(x + xoff) * freq,
					(float)(y + yoff) * freq,
					sqrtf((x + xoff) * (y + yoff)) * freq));
#endif
#if 1
			// pass 3 - cloud detail
			pix = (int)(gen_heightmap[y * HEIGHTMAP_SIZE + x]) +
				(int)(cloudmap[y * HEIGHTMAP_SIZE + x] / 2);
			if (pix < 0)
				pix = 0;
			else if (pix > 255)
				pix = 255;
			((char *)gen_heightmap)[y * HEIGHTMAP_SIZE + x] = pix;
#endif
		}
		g_loading_tick();
	}

	free(cloudmap);

	// since we've successfully generated a height map, dump it into a cache
	gen_save_terrain_cache(seed);
}

static float gen_sample_height(float x, float y) {
#if 1
	// bilinear filtering
	float xi, yi, xfrac, yfrac;
	float fR1, fR2;

	xfrac = modff(x, &xi);
	yfrac = modff(y, &yi);

	// bilinear filtering
	fR1 = (1.f - xfrac) * gen_heightmap[(int)yi * HEIGHTMAP_SIZE + (int)xi]
		+ xfrac * gen_heightmap[(int)yi * HEIGHTMAP_SIZE + (int)xi + 1];
	fR2 = (1.f - xfrac) * gen_heightmap[((int)yi + 1) * HEIGHTMAP_SIZE + (int)xi]
		+ xfrac * gen_heightmap[((int)yi + 1) * HEIGHTMAP_SIZE + (int)xi + 1];
	return ((1.f - yfrac) * fR1 + yfrac * fR2) * HEIGHT_SCALE;
#else
	// nearest filtering
	return gen_heightmap[(int)roundf(y) * HEIGHTMAP_SIZE + (int)roundf(x)]
		* HEIGHT_SCALE;
#endif
}

void gen_props(uchar *texture, ac_vertex_t *verts, uchar *indices) {
	int i, l, base, vofs, iofs;
	const float invScale = 1.f / (PROP_TEXTURE_SIZE - 1);
	float rpi;

	// generate vertices and indices

	// trees

	// vertex #0 - tip of the tree
	verts[0].pos = ac_vec_set(0, 1, 0, 0);
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;

	vofs = 1;
	iofs = 0;
	for (l = 0; l < 3; l++) {
		base = TREE_BASE - l * 2;
		rpi = (M_PI * 2.f) / (float)base;	// radians per iteration
		indices[iofs++] = 0;	// start with the tip of the tree
		// tree base vertices and indices
		for (i = 0; i < base; i++) {
			verts[vofs].pos = ac_vec_set(cosf(i * rpi), -0.1, sinf(i * rpi), 0);
			verts[vofs].st[0] = 0;
			verts[vofs].st[1] = 0;
			indices[iofs++] = vofs++;
		}
		indices[iofs++] = vofs - base;	// first base vertex - full circle
	}

	g_loading_tick();

	// buildings

	// flat roof
	l = vofs;
	for (i = 0; i < 8; i++, vofs++) {
		verts[vofs].pos = ac_vec_set(
			((i + 1) / 2) % 2 == 0 ? -0.5 : 0.5,
			i < 4 ? -1 : 1,
			(i / 2) % 2 == 0 ? -0.5 : 0.5,
			0);
		verts[vofs].st[0] = i % 2;
		verts[vofs].st[1] = i < 4 ? 1 : (1.f / (float)(PROP_TEXTURE_SIZE - 1));
		if (i < 4) {
			indices[iofs++] = l + i + 4;
			indices[iofs++] = l + i;
		}
	}
	// close the wall
	indices[iofs++] = l + 4;
	indices[iofs++] = l;
	// degenerate triangle
	indices[iofs] = indices[iofs - 1];
	iofs++;
	indices[iofs++] = l + 4;
	// rooftop
	indices[iofs++] = l + 4;
	indices[iofs++] = l + 5;
	indices[iofs++] = l + 7;
	indices[iofs++] = l + 6;

	// slanted roof
	l = vofs;
	// building base
	for (i = 0; i < 8; i++, vofs++) {
		verts[vofs].pos = ac_vec_set(
			((i + 1) / 2) % 2 == 0 ? -0.5 : 0.5,
			i < 4 ? -1 : 1,
			(i / 2) % 2 == 0 ? -0.5 : 0.5,
			0);
		verts[vofs].st[0] = i % 2;
		verts[vofs].st[1] = i < 4 ? 1 : (2.f / (float)(PROP_TEXTURE_SIZE - 1));
		if (i < 4) {
			indices[iofs++] = l + i + 4;
			indices[iofs++] = l + i;
		}
	}
	// close the wall
	indices[iofs++] = l + 4;
	indices[iofs++] = l;
	// roof verts
	verts[vofs].pos = ac_vec_set(0, 1.4, -0.5, 0);
	verts[vofs].st[0] = 0.5;
	verts[vofs++].st[1] = (1.f / (float)(PROP_TEXTURE_SIZE - 1));
	verts[vofs].pos = ac_vec_set(0, 1.4, 0.5, 0);
	verts[vofs].st[0] = 0.5;
	verts[vofs++].st[1] = (1.f / (float)(PROP_TEXTURE_SIZE - 1));
	// degenerate triangle
	indices[iofs] = indices[iofs - 1];
	iofs++;
	indices[iofs++] = l + 4;
	// north roof filler
	indices[iofs++] = l + 4;
	indices[iofs++] = l + 5;
	indices[iofs++] = l + 8;
	// degenerate triangle
	indices[iofs++] = l + 8;
	indices[iofs++] = l + 6;
	// south roof filler
	indices[iofs++] = l + 6;
	indices[iofs++] = l + 9;
	indices[iofs++] = l + 7;
	// degenerate triangle
	indices[iofs++] = l + 7;
	indices[iofs++] = l + 6;
	// rooftop
	indices[iofs++] = l + 7;
	indices[iofs++] = l + 4;
	indices[iofs++] = l + 9;
	indices[iofs++] = l + 8;
	indices[iofs++] = l + 6;
	indices[iofs++] = l + 5;

	// texture

	// tree strip: exponential gradient from gray to black
	for (i = 0; i < PROP_TEXTURE_SIZE; i++)
		texture[i] = 127 * expf(-2.f * i * invScale);
	// building roof - 2 pixels wide
	for (i = 0; i < PROP_TEXTURE_SIZE; i++)
		texture[PROP_TEXTURE_SIZE + i] = 20;
	for (i = 0; i < PROP_TEXTURE_SIZE; i++)
		texture[PROP_TEXTURE_SIZE * 2 + i] = 40;
	// building wall
	for (l = 3; l < PROP_TEXTURE_SIZE; l++) {
		for (i = 0; i < PROP_TEXTURE_SIZE; i++)
			texture[PROP_TEXTURE_SIZE * l + i] = 96 + gen_rand() % 32;
	}
	// window
	for (l = 0; l < 9; l++) {
		for (i = 0; i < 11; i++) {
			if (i == 5 || l == 4)
				continue;
			texture[(l + PROP_TEXTURE_SIZE / 2 - PROP_TEXTURE_SIZE / 3) * PROP_TEXTURE_SIZE
				+ i + PROP_TEXTURE_SIZE / 2 - 5] = 255;
		}
	}

	g_loading_tick();
}

static inline float gen_smoothstep(float t) {
	return t * t * (3 - 2 * t);
}

void gen_fx(uchar *texture, ac_vertex_t *verts, uchar *indices) {
	int i, j;
	float d, x, y, z, f, perlin;
	const float r = (FX_TEXTURE_SIZE - 1) * 0.5;
	const float invr = 1.f / r;
	const float invr2 = invr * invr;

	// geometry
	for (i = 0; i < 4; i++) {
		verts[i].pos = ac_vec_set(
			i % 2 == 0 ? -0.5 : 0.5,
			i < 2 ? 0.5 : -0.5,
			0,
			0);
		verts[i].st[0] = i % 2 == 0 ? 0 : 1;
		verts[i].st[1] = i < 2 ? 0 : 1;
		indices[i] = i;
	}

	// texture
	for (i = 0; i < FX_TEXTURE_SIZE; i++) {
		y = (float)i - r;
		for (j = 0; j < FX_TEXTURE_SIZE; j++) {
			x = (float)j - r;
			// this won't give us an exact sphere, but it's close enough
			z = -sinf(acosf(x * y * invr2));
			d = sqrtf(x * x + y * y);
			perlin = gen_perlin(x * invr * 4, y * invr * 4, z * invr * 4);
			texture[(i * FX_TEXTURE_SIZE + j) * 2 + 0] = 225 + perlin * 30;
			if (d <= r - 80.f)
				texture[(i * FX_TEXTURE_SIZE + j) * 2 + 1] = 255;
			else if (d <= r) {
				f = (r - d) / 80.f;
				perlin = (perlin + 1.f) * 0.5;
				texture[(i * FX_TEXTURE_SIZE + j) * 2 + 1] =
					gen_smoothstep(f) * 255 * (f + perlin * (1 - f));
			} else
				texture[(i * FX_TEXTURE_SIZE + j) * 2 + 1] = 0;
		}
		g_loading_tick();
	}
}

static uchar	*gen_propmap;

static void gen_propmap_populate(int x, int y, int *counter, int *trace,
									uchar value) {
	int k;

	if (x < 0 || x >= PROPMAP_SIZE || y < 0 || y >= PROPMAP_SIZE)
		return;
	if (!(*trace) || !(*counter))
		return;
	k = y * PROPMAP_SIZE + x;
	if (gen_propmap[k] != 0)
		return;
	gen_propmap[k] = value;
	(*trace)--;
	(*counter)--;
	if (!(*trace) || !(*counter))
		return;
	// try to recurse in one of the directions with a 33% chance
	if (gen_rand() % 100 > 33)
		gen_propmap_populate(x + 1, y, counter, trace, value);
	if (*counter && gen_rand() % 100 > 33)
		gen_propmap_populate(x + 1, y + 1, counter, trace, value);
	if (*counter && gen_rand() % 100 > 33)
		gen_propmap_populate(x, y + 1, counter, trace, value);
	if (*counter && gen_rand() % 100 > 33)
		gen_propmap_populate(x - 1, y + 1, counter, trace, value);
	if (*counter && gen_rand() % 100 > 33)
		gen_propmap_populate(x - 1, y, counter, trace, value);
	if (*counter && gen_rand() % 100 > 33)
		gen_propmap_populate(x - 1, y - 1, counter, trace, value);
	if (*counter && gen_rand() % 100 > 33)
		gen_propmap_populate(x, y - 1, counter, trace, value);
	if (*counter && gen_rand() % 100 > 33)
		gen_propmap_populate(x + 1, y - 1, counter, trace, value);
}

static void gen_create_propmap(void) {
	const int numFields = PROPMAP_SIZE * PROPMAP_SIZE;
	int treeFields = TREE_COVERAGE * numFields;
	int bldgFields = BLDG_COVERAGE * numFields;

	int trace;

	while (treeFields) {
		trace = 1 + gen_rand() % 10;
		gen_propmap_populate(gen_rand() % PROPMAP_SIZE,
								gen_rand() % PROPMAP_SIZE,
								&treeFields, &trace, 1);
	}

	g_loading_tick();

	while (bldgFields) {
		trace = 1 + gen_rand() % 4;
		gen_propmap_populate(gen_rand() % PROPMAP_SIZE,
								gen_rand() % PROPMAP_SIZE,
								&bldgFields, &trace, 2);
	}

	g_loading_tick();
}

static ac_prop_t *gen_recurse_propmap(int *numTrees, ac_tree_t *trees,
					int *numBldgs, ac_bldg_t *bldgs, int x, int y, int step) {
	int i;
	float tx, tz, min, max;
	ac_prop_t *node = NULL; // shut up compiler

	min = FLT_MAX;
	max = -FLT_MAX;

	if (step < 1) {
		float h;
		switch (gen_propmap[y * PROPMAP_SIZE + x]) {
			case 0:
				return NULL;
			case 1:	// tree node
				node = calloc(1, sizeof(ac_prop_t));
				node->trees = &trees[*numTrees];
				for (i = 0; i < TREES_PER_FIELD; i++) {
					tx = (x << PROPMAP_SHIFT) + 0.01 * (gen_rand()
						% ((1 << PROPMAP_SHIFT) * 100));
					tz = (y << PROPMAP_SHIFT) + 0.01 * (gen_rand()
						% ((1 << PROPMAP_SHIFT) * 100));
					h = gen_sample_height(tx, tz);
					trees[i + *numTrees].pos = ac_vec_set(
						tx - HEIGHTMAP_SIZE * 0.5,
						h,
						tz - HEIGHTMAP_SIZE * 0.5,
						1.f);
					trees[i + *numTrees].ang =
						(gen_rand() % 360) / 180.f * M_PI;
					trees[i + *numTrees].XZscale =
						1.0 + 0.001 * (gen_rand() % 1201);
					trees[i + *numTrees].Yscale =
						2.4 + 0.001 * (gen_rand() % 3201);
					if (h - 0.1 < min)
						min = h - 0.1;
					else if (h + trees[i + *numTrees].Yscale > max)
						max = h + trees[i + *numTrees].Yscale;
				}
				(*numTrees) += TREES_PER_FIELD;
				break;
			case 2:	// building node
				node = calloc(1, sizeof(ac_prop_t));
				node->bldgs = &bldgs[*numBldgs];
				for (i = 0; i < BLDGS_PER_FIELD; i++) {
					tx = (x << PROPMAP_SHIFT) + 0.01 * (gen_rand()
						% ((1 << PROPMAP_SHIFT) * 100));
					tz = (y << PROPMAP_SHIFT) + 0.01 * (gen_rand()
						% ((1 << PROPMAP_SHIFT) * 100));
					h = gen_sample_height(tx, tz);
					bldgs[i + *numBldgs].pos = ac_vec_set(
						tx - HEIGHTMAP_SIZE * 0.5,
						h,
						tz - HEIGHTMAP_SIZE * 0.5,
						1.f);
					bldgs[i + *numBldgs].ang =
						((gen_rand() % 4) * 90 - 10 + gen_rand() % 21)
							/ 180.f * M_PI;
					bldgs[i + *numBldgs].Xscale =
						5 + 0.001 * (gen_rand() % 2001);
					bldgs[i + *numBldgs].Zscale =
						7 + 0.001 * (gen_rand() % 3001);
					bldgs[i + *numBldgs].Yscale =
						2.8 + 0.001 * (gen_rand() % 3001);
					bldgs[i + *numBldgs].slantedRoof =
						gen_rand() % 100 >= 33;
					if (h - 1 < min)
						min = h - 1;
					/*else if (h + bldgs[i + *numBldgs].Yscale > max)
						max = h + bldgs[i + *numBldgs].Yscale;*/
				}
				// HACK: for some reason the calculated heights are insufficient
				max = HEIGHT + 5.8;
				(*numBldgs) += BLDGS_PER_FIELD;
				break;
		}
	} else {
		ac_prop_t *c1, *c2, *c3, *c4;
		c1 = gen_recurse_propmap(numTrees, trees, numBldgs, bldgs,
								x, y, step / 2);
		c2 = gen_recurse_propmap(numTrees, trees, numBldgs, bldgs,
								x + step, y, step / 2);
		c3 = gen_recurse_propmap(numTrees, trees, numBldgs, bldgs,
								x, y + step, step / 2);
		c4 = gen_recurse_propmap(numTrees, trees, numBldgs, bldgs,
								x + step, y + step, step / 2);
		if (!c1 && !c2 && !c3 && !c4)
			// no children, no point in creating a node
			return NULL;
		node = calloc(1, sizeof(ac_prop_t));
		// don't try to check the bounds of NULL children
		min = ac_min(c1 ? c1->bounds[0].f[1] : FLT_MAX,
			ac_min(c2 ? c2->bounds[0].f[1] : FLT_MAX,
				ac_min(c3 ? c3->bounds[0].f[1] : FLT_MAX,
					c4 ? c4->bounds[0].f[1] : FLT_MAX)));
		max = ac_max(c1 ? c1->bounds[1].f[1] : -FLT_MAX,
			ac_max(c2 ? c2->bounds[1].f[1] : -FLT_MAX,
				ac_max(c3 ? c3->bounds[1].f[1] : -FLT_MAX,
					c4 ? c4->bounds[1].f[1] : -FLT_MAX)));
		node->child[0] = c1;
		node->child[1] = c2;
		node->child[2] = c3;
		node->child[3] = c4;
	}
	node->bounds[0] = ac_vec_set(
		(x << PROPMAP_SHIFT) - HEIGHTMAP_SIZE / 2,
		min,
		(y << PROPMAP_SHIFT) - HEIGHTMAP_SIZE / 2,
		0);
	node->bounds[1] = ac_vec_set(
		((x + step * 2) << PROPMAP_SHIFT) - HEIGHTMAP_SIZE / 2,
		max,
		((y + step * 2) << PROPMAP_SHIFT) - HEIGHTMAP_SIZE / 2,
		0);
	return node;
}

void gen_proplists(int *numTrees, ac_tree_t *trees,
					int *numBldgs, ac_bldg_t *bldgs) {
	gen_propmap = malloc(sizeof(*gen_propmap) * PROPMAP_SIZE * PROPMAP_SIZE);
	memset(gen_propmap, 0, sizeof(*gen_propmap) * PROPMAP_SIZE * PROPMAP_SIZE);

	*numTrees = 0;
	*numBldgs = 0;

	// generate a propmap to roughly place clusters of objects
	gen_create_propmap();

	// use the propmap to place the actual objects
	gen_proptree = gen_recurse_propmap(numTrees, trees, numBldgs, bldgs,
							0, 0, PROPMAP_SIZE / 2);

	g_loading_tick();

	free(gen_propmap);
}

void gen_free_proptree(ac_prop_t *n) {
	int i;
	if (n == NULL)
		n = gen_proptree;
	// it's enough to just check the existence of the 1st child because a node
	// will always either have 4 children or none
	if (n->trees == NULL && n->bldgs == NULL) {
		for (i = 0; i < 4; i++) {
			if (n->child[i])
				gen_free_proptree(n->child[i]);
		}
	}
	free(n);
}
