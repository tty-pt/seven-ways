#include <ttypt/lam.h>
#include "../include/view.h"
#include "../include/gtime.h"
#include "../include/tile.h"
#include "../include/char.h"
#include "../include/font.h"

void png_init(void);
uint8_t dim = 3;

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
	lam_flush();
	dt = dt_get();
	view_update(dt);
	lam_poll();
	return dt;
}

void
game_start(void)
{
	time_init();
}
