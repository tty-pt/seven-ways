#include <ttypt/qgl-tm.h>
#include "../include/char.h"
#include "../include/tile.h"
#include "../include/time.h"
#include "../include/cam.h"
#include "../include/view.h"
#include "../include/dialog.h"

#include <math.h>

#include <ttypt/qmap.h>

typedef struct {
	unsigned tm_ref;
	double pos[4];
	double x, y, nx, ny;
	uint8_t anim, dir;
	unsigned dialog;
} char_t;

extern cam_t cam;

static unsigned char_hd;
static double char_speed = 7.0;

static uint8_t anim_frames[] = {
	[AN_WALK] = 0,
	[AN_IDLE] = 4,
};

void char_render(unsigned ref)
{
	const char_t *ch = qmap_get(char_hd, &ref);
	const qgl_tm_t *tm = qgl_tm_get(ch->tm_ref);

	/* nº de frames na linha (como antes) */
	uint8_t anim_n = anim_frames[AN_IDLE] ? anim_frames[AN_IDLE] : tm->nx;
	uint32_t xn = ((uint32_t)(time_tick * char_speed)) % anim_n;

	/* (col=xn, row=ch->anim*4 + ch->dir) → idx linear */
	uint32_t row = ch->anim * 4 + ch->dir;
	uint32_t idx = row * tm->nx + xn;

	double offs = tm->w / 32.0;

	int32_t scr_x = (int32_t)(view_hw + (ch->x + ch->nx - offs + 0.5 - cam.x) * view_mul);
	int32_t scr_y = (int32_t)(view_hh + (ch->y + ch->ny - offs - cam.y) * view_mul);

	/* ordem correta: (ref, idx, x, y, w, h, rx, ry) */
	qgl_tile_draw(ch->tm_ref, idx,
	              scr_x, scr_y,
	              (uint32_t)(cam.zoom * tm->w),
	              (uint32_t)(cam.zoom * tm->h),
	              1, 1);
}

void
char_face(unsigned ref, enum dir dir)
{
	char_t *ch = (char_t *)
		qmap_get(char_hd, &ref);

	ch->dir = dir;
}

void
char_animate(unsigned ref, enum anim anim)
{
	char_t *ch = (char_t *)
		qmap_get(char_hd, &ref);

	ch->anim = anim;
}

enum dir
char_dir(unsigned ref)
{
	const char_t *ch = qmap_get(char_hd, &ref);
	return ch->dir;
}

enum anim
char_animation(unsigned ref)
{
	const char_t *ch = qmap_get(char_hd, &ref);
	return ch->anim;
}

void
char_ipos(int16_t *p, unsigned ref)
{
	const char_t *ch = qmap_get(char_hd, &ref);

	p[0] = ch->x;
	p[1] = ch->y;
	p[2] = 0;
}

void
char_pos(double *x, double *y, unsigned ref)
{
	const char_t *ch = qmap_get(char_hd, &ref);

	*x = ch->x + ch->nx;
	*y = ch->y + ch->ny;
}

int
char_update(unsigned ref, double dt)
{
	char_t *ch = (char_t *) qmap_get(char_hd, &ref);
	char_t cho;
	double char_speed = 4.0, tr;

	if (ch->anim == AN_IDLE)
		return 1;

	tr = dt * char_speed;

	if (view_collides(ch->x, ch->y, ch->dir) != QM_MISS)
	{
		ch->anim = AN_IDLE;
		return 0;
	}

	switch (ch->dir) {
		case DIR_UP:
			ch->ny -= tr;
			break;
		case DIR_DOWN:
			ch->ny += tr;
			break;
		case DIR_LEFT:
			ch->nx -= tr;
			break;
		case DIR_RIGHT:
			ch->nx += tr;
			break;
	}

	if (fabs(ch->nx) < 1.0 && fabs(ch->ny) < 1.0)
		return 1;

	ch->x = round(ch->x + ch->nx);
	ch->y = round(ch->y + ch->ny);
	ch->nx = ch->ny = 0;
	ch->anim = AN_IDLE;
	cho = *ch;

	qmap_del(char_hd, &ref);
	qmap_put(char_hd, &ref, &cho);

	return 0;
}

unsigned
char_load(unsigned tm_ref, double x, double y) {
	char_t ch = {0};
	unsigned ret;

	ch.tm_ref = tm_ref;
	ch.x = x;
	ch.y = y;
	ch.anim = AN_IDLE;
	ch.dir = DIR_DOWN;
	ch.dialog = QM_MISS;

	ret = qmap_put(char_hd, NULL, &ch);
	vchar_put(ret, x, y);
	return ret;
}

unsigned
char_dialog(unsigned ref, char *text)
{
	char_t *ch = (char_t *)
		qmap_get(char_hd, &ref);

	unsigned dialog = dialog_add(text);
	ch->dialog = dialog;
	return dialog;
}

void char_talk(unsigned ref, enum dir dir) {
	const char_t *ch = qmap_get(char_hd, &ref);
	static enum dir reverse_dir[] = {
		DIR_UP,
		DIR_DOWN,
		DIR_RIGHT,
		DIR_LEFT,
	};

	dialog_start(ch->dialog);
	char_face(ref, reverse_dir[dir]);
}

void
char_sync(void)
{
}

void
char_init(void)
{
	unsigned qm_char = qmap_reg(sizeof(char_t));

	char_hd = qmap_open(NULL, NULL, QM_HNDL, qm_char, 0xFF, QM_AINDEX);
}
