#include <ttypt/qgl.h>
#include "../include/view.h"
#include "../include/time.h"
#include "../include/tile.h"
#include "../include/char.h"
#include "../include/font.h"

void png_init(void);
const uint8_t dim = 3;

void
game_init(void)
{
	view_init();
	char_init();
}

double
game_update(void) {
	double dt;
	/* be_flush(); */
	qgl_flush();
	dt = dt_get();
	view_update(dt);
	qgl_poll();
	return dt;
}

void
game_start(void)
{
	time_init();
}
