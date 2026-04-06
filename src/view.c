#include <ttypt/qgl.h>
#include <ttypt/qgl-tm.h>
#include "../include/tile.h"
#include "../include/cam.h"
#include "../include/char.h"
#include "../include/dialog.h"
#include "../include/map.h"

#include <string.h>
#include <stdio.h>

#include <ttypt/qsys.h>
#include <ttypt/qmap.h>
#include <ttypt/geo.h>
#include <ttypt/point.h>

typedef struct {
	unsigned bm_ref, tm_ref;
} layer_t;

static unsigned me = 0;
extern uint8_t dim;
unsigned smap_hd;
cam_t cam;
unsigned floors_tm;
double view_mul,
       view_w, view_h,
       view_hw, view_hh;

unsigned layer_n, above_n, ts_n;
layer_t *view_layers;
layer_t *view_above;
unsigned *view_flags;
unsigned view_tms[4];
uint32_t map_width, map_height;

int16_t view_min[3], view_max[3];

void view_tl(int16_t tl[dim], uint8_t ow) {
	uint16_t n_ow = ow / 16;

	tl[0] = cam.x - view_w / 2 - n_ow;
	tl[1] = cam.y - view_h / 2 - n_ow;
	tl[2] = 0;
}

void view_len(uint16_t l[dim], uint8_t ow) {
	uint16_t n_ow = ow / 16;

	l[0] = view_w + 2 + 2 * n_ow;
	l[1] = view_h + 2 + 2 * n_ow;
	l[2] = 4;
}

static inline void
view_bmtl(uint32_t *target, int16_t *tl)
{
	target[0] = tl[0] - view_min[0];
	target[1] = tl[1] - view_min[1];
}

static inline void
view_render_tile(int16_t *tl, uint32_t *bmtl,
		uint32_t x, uint32_t y, uint32_t z, layer_t *layers)
{
	int16_t p[] = { x + tl[0], y + tl[1], z };
	layer_t *layer = &layers[z];
	uint32_t col = qgl_tex_pick(layer->bm_ref,
			x + bmtl[0],
			y + bmtl[1]);

	unsigned idx = map_idx(col);
	if (idx)
		tile_render(layer->tm_ref, idx, p);
}

static inline void
view_render_bm(int16_t *tl, uint32_t *bmtl, uint16_t *len,
		unsigned layer_idx UNUSED,
		unsigned layer_n, unsigned layer_off)
{
	for (uint32_t y = 0; y < len[1]; y++)
		for (uint32_t x = 0; x < len[0]; x++)
			for (uint32_t z = 0; z < layer_n;
					z++)
				view_render_tile(tl, bmtl,
						x, y, z, view_layers + layer_off);
}

void
view_render(void) {
	unsigned ref;
	int16_t p[dim];
	unsigned cur;
	uint16_t l[dim];
	int16_t tl[dim];
	uint32_t bmtl[dim];

	view_tl(tl, 16);
	view_bmtl(bmtl, tl);
	view_len(l, 16);

	view_render_bm(tl, bmtl, l, 0, layer_n, 0);

	cur = geo_iter(smap_hd, tl, l, dim);
	while (geo_next(p, &ref, cur))
		char_render(ref);

	view_render_bm(tl, bmtl, l, 0, above_n, layer_n);
}

void
vchar_put(unsigned ref, int16_t x, int16_t y)
{
	int16_t s[] = { x, y, 0, 0 };
	geo_put(smap_hd, s, ref, dim);
}

void
view_load(char *filename) {
	char line[BUFSIZ];
	FILE *fp = fopen(filename, "r");
	char *space, *word, *ret;

	CBUG(!fp, "couldn't open %s\n", filename);

	while (1) {
		uint16_t w;

		ret = fgets(line, sizeof(line), fp);
		word = line;

		CBUG(!ret, "file input end: A\n");

		if (*line == '\n')
			break;

		w = strtold(word, &word);

		word++;
		space = strchr(word, '\n');
		CBUG(!space, "file input end: C\n");
		*space = '\0';

		unsigned img = qgl_tex_load(word);
		qgl_tm_new(img, w, w);
	}

#if !CHAR_SYNC
	while (fgets(line, sizeof(line), fp)) {
		unsigned ref;
		uint16_t x, y;

		word = line;

		sscanf(line, "%u %hu %hu", &ref, &x, &y);
		char_load(ref, x, y);
	}
#endif

	fclose(fp);

	fp = fopen("./map/info.txt", "r");
	ret = fgets(line, sizeof(line), fp);
	word = line;

	map_width = strtoull(word, &word, 10);
	map_height = strtoull(word, &word, 10);
	layer_n = strtoull(word, &word, 10);
	above_n = strtoull(word, &word, 10);
	ts_n = strtoull(word, &word, 10);

	view_min[0] = -map_width / 2;
	view_min[1] = -map_height / 2;
	view_min[2] = 0;

	fclose(fp);

	view_layers = malloc((layer_n + above_n) * sizeof(layer_t));

	for (unsigned i = 0; i < layer_n + above_n; i++) {
		layer_t *layer = &view_layers[i];
		snprintf(line, sizeof(line),
				"./map/map%u.png",
				i);
		layer->bm_ref = qgl_tex_load(line);
		layer->tm_ref = 0;
	}

	unsigned col_ref = qgl_tex_load("./map/col0.png");
	size_t len = map_width * map_height * sizeof(unsigned);

	view_flags = malloc(len);
	memset(view_flags, 0, len);

	const qgl_tm_t *tm = qgl_tm_get(0);
	unsigned *vf = view_flags;

	for (uint32_t y = 0; y < map_height; y++)
		for (uint32_t x = 0; x < map_width; x++, vf++)
			for (uint32_t z = 0; z < layer_n; z++) {
				layer_t *layer = &view_layers[z];
				uint32_t color = qgl_tex_pick(
						layer->bm_ref, x, y);
				unsigned idx = map_idx(color);
				uint32_t tx = idx % tm->nx,
					 ty = idx / tm->nx;
				uint32_t col = qgl_tex_pick(col_ref, tx, ty);
				*vf |= col;
			}
}

void
view_init(void)
{
	uint32_t be_width, be_height;
	geo_init();

	qgl_size(&be_width, &be_height);

	cam.x = 0;
	cam.y = 0;
	cam.zoom = 4;
	view_mul = 16.0 * cam.zoom;
	view_hw = 0.5 * ((double) be_width - view_mul);
	view_hh = 0.5 * ((double) be_height - view_mul);
	view_w = be_width / view_mul;
	view_h = be_height / view_mul;

	smap_hd = geo_open(NULL, NULL, 0xFFFFF);
	qmap_drop(smap_hd);
}

static inline void
vchar_update(unsigned ref, double dt)
{
	int16_t p[] = { 0, 0, 0, 0 };

	char_ipos(p, ref);

	if (char_update(ref, dt))
		return;

	geo_del(smap_hd, p, dim);
	char_ipos(p, ref);
	geo_put(smap_hd, p, ref, dim);
}

void
view_paint(unsigned ref, unsigned tile, uint16_t layer)
{
	double x, y;

	char_pos(&x, &y, ref);
	qgl_tex_paint(view_layers[layer].bm_ref,
			((int16_t) x) - view_min[0],
			((int16_t) y) - view_min[1],
			map_color(tile));
}

unsigned
view_collides(double x, double y, enum dir dir)
{
	switch (dir) {
		case DIR_UP:
			y -= 1;
			break;
		case DIR_DOWN:
			y += 1;
			break;
		case DIR_LEFT:
			x -= 1;
			break;
		case DIR_RIGHT:
			x += 1;
	}

	int16_t p[] = { x, y, 0, 0 };
	unsigned ret = geo_get(smap_hd, p, dim);
	
	if (ret != QM_MISS)
		return ret;

	uint32_t mx = x - view_min[0], my = y - view_min[1];

	if (view_flags[my * map_width + mx] & 0x1)
		return 0;

	return QM_MISS;
}

void
view_update(double dt)
{
	unsigned cur = qmap_iter(smap_hd, NULL, 0);
	const void *key, *value;

	while (qmap_next(&key, &value, cur))
		vchar_update(* (unsigned *) value, dt);
}

void
view_sync(void)
{
	unsigned tile = 0;
	const qgl_tm_t *tm = qgl_tm_get(view_layers[1].tm_ref);
	
	for (uint32_t y = 0; y < tm->ny; y++) {
		for (uint32_t x = 0; x < tm->nx; x++, tile++) {
			qgl_tex_paint(view_layers[1].bm_ref,
					x, y,
					map_color(tile));
		}
	}


	qgl_tex_save(view_layers[0].bm_ref);
	qgl_tex_save(view_layers[1].bm_ref);
}

int
vdialog_action(void)
{
	double x, y;
	unsigned npc;
	enum dir dir = char_dir(me);

	if (dialog_action())
		return 1;

	char_pos(&x, &y, me);
	npc = view_collides(x, y, dir);

	if (npc == QM_MISS || npc == 0)
		return 0;

	char_talk(npc, dir);
	return 1;
}
