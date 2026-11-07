#include <ttypt/lam.h>
#include <ttypt/lam-tm.h>
#include <ttypt/lam-cam.h>
#include "../include/tile.h"
#include "../include/cam.h"
#include "../include/view.h"

#include <ttypt/stele.h>

extern cam_t cam;

void
tile_render(uint32_t tm_ref, uint32_t idx, int16_t *p)
{
	if (cam.mode_3d) {
		lam_tile_draw_ground_3d(tm_ref, idx, (float)p[0], (float)p[1], (float)p[2], 1.0f, 1.0f);
		return;
	}

	int32_t x = view_hw + (int32_t)((p[0] - cam.x) * view_mul);
	int32_t y = view_hh + (int32_t)((p[1] - cam.y) * view_mul);

	uint32_t dw = (uint32_t)view_mul;
	uint32_t dh = (uint32_t)view_mul;

	lam_tile_draw(tm_ref, idx, x, y, dw, dh, 1, 1);
}
