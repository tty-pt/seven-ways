#include <ttypt/qgl.h>
#include <ttypt/qgl-tm.h>   /* usa o header certo do TM */
#include "../include/tile.h"
#include "../include/cam.h"
#include "../include/view.h"

#include <ttypt/qsys.h>

extern cam_t cam;

void
tile_render(uint32_t tm_ref, uint32_t idx, int16_t *p)
{
	int32_t x = view_hw + (int32_t)((p[0] - cam.x) * view_mul);
	int32_t y = view_hh + (int32_t)((p[1] - cam.y) * view_mul);

	uint32_t dw = (uint32_t)view_mul;
	uint32_t dh = (uint32_t)view_mul;

	qgl_tile_draw(tm_ref, idx, x, y, dw, dh, 1, 1);
}
