#include <ttypt/qgl.h>
#include <ttypt/qgl-tm.h>
#include <ttypt/qgl-ui.h>
#include <ttypt/qgl-font.h>

#include "../include/game.h"
#include "../include/dialog.h"
#include "../include/time.h"

#include <stdio.h>
#include <stdlib.h>

#include <ttypt/qsys.h>

unsigned start_cont = 1, cont = 1;
unsigned lamb;
unsigned tile, layer = 0;
unsigned dlg_edit, dlg_quit, dlg_save;
unsigned font_ref;

extern double hw, hh;

void quit_nosave(void) {
	cont = 0;
}

void quit_save(void) {
	view_sync();
	char_sync();
	cont = 0;
}

int key_quit(unsigned short code UNUSED,
		unsigned short type UNUSED,
		int value UNUSED)
{
	dialog_start(dlg_quit);
	return 0;
}

int key_cont(unsigned short code UNUSED,
		unsigned short type,
		int value UNUSED)
{
	if (type) {
		if (start_cont) {
			start_cont = 0;
			return 1;
		}

		return 0;
	}

	if (vdialog_action())
		return 1;

	view_paint(lamb, tile, layer);
	return 0;
}

static inline int
key_dialog_sel(unsigned short type, int down)
{
	if (type)
		return 0;

	return dialog_select(down);
}

int key_up(unsigned short code UNUSED,
		unsigned short type, int value UNUSED)
{
	return key_dialog_sel(type, 0);
}

int key_down(unsigned short code UNUSED,
		unsigned short type, int value UNUSED)
{
	return key_dialog_sel(type, 1);
}

int dlg_key(unsigned short code UNUSED,
		unsigned short type, unsigned dlg)
{
	if (type || dialog_showing())
		return 0;

	dialog_start(dlg);
	return 1;
}

int key_edit(unsigned short code,
		unsigned short type,
		int value UNUSED)
{
	char **args = dialog_args();
	static char tile_s[BUFSIZ], layer_s[BUFSIZ];
	sprintf(tile_s, "%u", tile);
	sprintf(layer_s, "%u", layer);
	args[0] = tile_s;
	args[1] = layer_s;

	return dlg_key(code, type, dlg_edit);
}

void tile_sel(void) {
	char **args = dialog_args();
	tile = strtoull(args[0], NULL, 10);
}

void layer_sel(void) {
	char **args = dialog_args();
	layer = strtoull(args[0], NULL, 10);
}

void ensure_move(enum dir dir) {
	if (dialog_showing())
		return;

	if (char_animation(lamb) == AN_WALK)
		return;

	char_face(lamb, dir);
	char_animate(lamb, AN_WALK);
}

void
my_update(void) {
	if (qgl_key_val(QGL_KEY_J) || qgl_key_val(QGL_KEY_DOWN))
		ensure_move(DIR_DOWN);
	if (qgl_key_val(QGL_KEY_K) || qgl_key_val(QGL_KEY_UP))
		ensure_move(DIR_UP);
	if (qgl_key_val(QGL_KEY_H) || qgl_key_val(QGL_KEY_LEFT))
		ensure_move(DIR_LEFT);
	if (qgl_key_val(QGL_KEY_L) || qgl_key_val(QGL_KEY_RIGHT))
		ensure_move(DIR_RIGHT);
}

int qgl_key(unsigned short code,
		unsigned short value,
		int type UNUSED)
{
	if (value)
		return 0;
	else
		return input_press(code);
}

int main(void) {
	uint32_t w, h;

	qgl_size(&w, &h);
	qui_init(w, h);

	game_init();

	qgl_key_reg(QGL_KEY_ENTER, key_cont);
	qgl_key_reg(QGL_KEY_SPACE, key_cont);
	qgl_key_reg(QGL_KEY_TAB, key_cont);

	qgl_key_reg(QGL_KEY_Q, key_quit);

	qgl_key_reg(QGL_KEY_J, key_down);
	qgl_key_reg(QGL_KEY_DOWN, key_down);
	qgl_key_reg(QGL_KEY_K, key_up);
	qgl_key_reg(QGL_KEY_UP, key_up);

	qgl_key_reg(QGL_KEY_E, key_edit);

	qgl_key_default_reg(qgl_key);

	view_load("./map.txt");

	unsigned entry_ref = qgl_tex_load("./resources/seven.png");
	unsigned press_ref = qgl_tex_load("./resources/press.png");
	font_ref = qgl_font_open("./resources/font.png", 9, 16, 0, 255); 
	dialog_init();

	lamb = 0;

	dlg_quit = dialog_add("Really quit?");
	dialog_option(dlg_quit, "No", NULL);
	unsigned dlg_save_ask
		= dialog_option(dlg_quit, "Yes",
				"Save?");
	unsigned dlg_nosave
		= dialog_option(dlg_save_ask, "No",
				"See you later.");
	unsigned dlg_save
		= dialog_option(dlg_save_ask, "Yes",
				"Saving. Bye-bye.");

	dialog_then(dlg_nosave, quit_nosave);
	dialog_then(dlg_save, quit_save);

	dlg_edit = dialog_add("Select your edit action");

	unsigned dlg_tile
		= dialog_option(dlg_edit,
				"Tile",
				"Select tile number.");
	dialog_input(dlg_tile, 7, 1, QGL_INPUT_NUMERIC,
				"Tile '%1' selected");
	dialog_then(dlg_tile, tile_sel);

	unsigned dlg_layer
		= dialog_option(dlg_edit,
				"Layer",
				"Select layer number.");
	dialog_input(dlg_layer, 7, 1, QGL_INPUT_NUMERIC,
			"Layer '%1' selected");
	dialog_then(dlg_layer, layer_sel);

	dialog_option(dlg_edit, "Info",
			"Tile: %1; Layer: %2");

	unsigned dlg = char_dialog(1, "Do you want to learn how to edit the map?");

	dialog_option(dlg, "No", "Oh. Ok, then. Smarty-pants.");
	unsigned dlg2 = dialog_option(dlg, "Yes", "Press 'e' to pick an edit action! You can place a tile in front of you using 'p'. If you want to paint the floor where you stand, use 'space'!\nDo you want to learn about editing dialog?");

	dialog_option(dlg2, "No", "Ok. Go ahead and paint!");
	dialog_option(dlg2, "Yes", "Press 't' to add a new dialog. Insert the text and when you are finished press 'tab'. The dialog ID will be shown. Options are similar. You press 'o' to add a new one, and you will be asked to input a dialog ID to associate it to. Pressing 'tab', you'll be asked to input the option's text. Doing it a third time, you'll be asked to insert the resulting dialog. Again, you'll get a dialog ID.\nGot all that?");

	game_start();

	uint32_t be_width, be_height;
	qgl_size(&be_width, &be_height);

	while (start_cont) {
		qgl_tex_draw(entry_ref,
				0, 0,
				be_width,
				be_height);


		uint8_t alpha = (time_tick * 200.0);

		qgl_tint(0x00FFFFFF
				| (((uint64_t) alpha) << 24));

		qgl_tex_draw(press_ref,
				be_width / 2 - 128,
				be_height - 128,
				256,
				118);

		qgl_tint(qgl_default_tint);

		game_update();
	}

	while (cont) {
		view_render();
		dialog_render();
		game_update();
		my_update();

		char_pos(&cam.x, &cam.y, lamb);
	}

	return 0;
}
