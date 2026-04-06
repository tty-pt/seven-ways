#include <ttypt/qgl.h>
#include <ttypt/qgl-font.h>
#include <ttypt/qgl-ui.h>
#include "../include/dialog.h"
#include "../include/time.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <ttypt/qmap.h>
#include <ttypt/idm.h>
#include <ttypt/qsys.h>

struct dialog {
	char *text;
	char *next;
	ids_t options;
	uint8_t option;
	uint8_t option_n;
	unsigned input;
	dialog_cb_t *then;
};

struct option {
	char *text;
	unsigned ref;
};

struct input {
	char text[BUFSIZ];
	uint32_t cw, ch;
	unsigned next, len, flags;
};

static unsigned dialog_hd, option_hd, input_hd;
static uint32_t be_width, be_height;

dialog_settings_t dialog = { .font_scale = 5 };
static uint32_t g_css;

static qui_div_t *root, *txt, *options, *cursor, *input;
static qui_div_t *menu_panel;
static struct dialog cdialog;
static char *dialog_arg[8];
static unsigned dialog_arg_n;
static ids_t dialog_seq;

extern uint32_t font_ref;
static char *arrow = "\x1F";
int cursor_on = 0;
int blink_state = 0;

static char *dialog_snprintf(char *fmt)
{
	static char res[BUFSIZ * 2];
	char *s, *r, *e;
	unsigned n;

	for (s = fmt, r = res; *s; r++, s++) {
		if (*s == '%' && isdigit((uint8_t)*(s + 1))) {
			n = (unsigned)strtoull(s + 1, &e, 10);
			if (n == 0) {
				*r = *s;
				continue;
			}
			const char *ins = dialog_arg[n - 1];
			size_t len = strlen(ins);
			memcpy(r, ins, len);
			r += len - 1;
			s = e - 1;
			continue;
		}
		*r = *s;
	}

	*r = '\0';
	return res;
}

#if 0
static void make_example(qui_div_t *parent, char *className) {
	qui_div_t *example = qui_new(parent, NULL);
	qui_class(example, className);

	for (int i = 0; i < 3; i++) {
		qui_div_t *row = qui_new(example, NULL);
		qui_text(row, "t");
		qui_class(row, "option-active");
	}
}
#endif

static qui_div_t *build_qui_tree(void)
{
	root = qui_new(NULL, NULL);

	qui_div_t *overlay = qui_new(root, NULL);
	qui_class(overlay, "overlay");

	menu_panel = qui_new(overlay, NULL);
	qui_class(menu_panel, "menu_panel");

	qui_div_t *panel = qui_new(overlay, NULL);
	qui_class(panel, "panel");

	/* make_example(overlay, "example_panel"); */
	/* make_example(overlay, "example_panel1"); */
	/* make_example(overlay, "example_panel2"); */
	/* make_example(overlay, "example_panel3"); */

	txt = qui_new(panel, NULL);
	qui_class(txt, "text");

	cursor = qui_new(panel, NULL);
	qui_text(cursor, arrow);
	qui_class(cursor, "cursor");

	options = qui_new(menu_panel, NULL);
	qui_class(options, "options");

	input = qui_new(menu_panel, NULL);
	qui_class(input, "input");
	return root;
}

static void update_options(void) {
	qui_clear(options);

	if (!cdialog.option_n)
		return;

	idsi_t *it = ids_iter(&cdialog.options);
	unsigned ref, idx = 0;

	for (; ids_next(&ref, &it); idx++) {
		const struct option *opt = qmap_get(option_hd, &ref);
		qui_div_t *row = qui_new(options, NULL);

		qui_class(row, (idx == cdialog.option) ?
				"option-active" : "option");
		qui_apply_styles(row, g_css);
		qui_text(row, opt->text);
	}
}

static void dialog_build_styles(void);

void qui_rebuild(void) {
	update_options();
	cursor_on = 1;

	qui_layout(root);

	cdialog.next = (char *)qui_overflow(txt);

	if (cdialog.next) {
		QUI_STYLE(options, display, QUI_DISPLAY_NONE);
		QUI_STYLE(cursor, display, QUI_DISPLAY_NONE);
		return;
	}

	if (cursor_on) {
		QUI_STYLE(cursor, display, QUI_DISPLAY_NONE);
		cursor_on = 0;
	}

	if (cdialog.option_n)
		QUI_STYLE(options, display, QUI_DISPLAY_BLOCK);

	if (cdialog.input != QM_MISS)
		QUI_STYLE(input, display, QUI_DISPLAY_BLOCK);
}

static void dialog_begin(unsigned ref)
{
	const struct dialog *dlg = qmap_get(dialog_hd, &ref);

	if (cdialog.text && cdialog.then)
		cdialog.then();
	if (!dlg)
		return;

	ids_push(&dialog_seq, ref);
	cdialog = *dlg;
	cdialog.text = dialog_snprintf(dlg->text);
	qui_text(txt, cdialog.text);
	qui_rebuild();
}

static void css_reset(qui_style_t *s)
{
	qui_style_reset(s);
	s->font_size = 5;
}

static void dialog_build_styles(void)
{
	g_css = qui_stylesheet_init();

	qui_style_t s;

	css_reset(&s);
	s.position = QUI_POSITION_ABSOLUTE;
	s.left = s.right = s.top = s.bottom = 0;
	s.padding_top = s.padding_bottom
		= s.padding_left = s.padding_right = 10;
	s.flex_direction = QUI_COLUMN;
	s.display = QUI_DISPLAY_FLEX;
	qui_stylesheet_add(g_css, "overlay", &s);

	css_reset(&s);
	s.background_color = 0xAA101010;
	s.align_items = QUI_ALIGN_STRETCH;
	s.border_color = 0xFFFFFFFF;
	s.border_width = 2;
	s.flex_direction = QUI_COLUMN;
	s.display = QUI_DISPLAY_NONE;
	s.box_shadow_offset_x = 10;
	s.box_shadow_offset_y = 10;
	s.box_shadow_blur = 10;
	s.box_shadow_color = 0x99000000;
	qui_stylesheet_add(g_css, "options", &s);

	css_reset(&s);
	s.background_color = 0xAA101010;
	s.border_color = 0xFFFFFFFF;
	s.border_width = 1;
	s.font_size = 1;
	s.flex_direction = QUI_ROW;
	s.align_items = QUI_ALIGN_STRETCH;
	s.display = QUI_DISPLAY_FLEX;
	s.justify_content = QUI_JUSTIFY_SPACE_AROUND;
	qui_stylesheet_add(g_css, "example_panel", &s);
	s.justify_content = QUI_JUSTIFY_SPACE_BETWEEN;
	qui_stylesheet_add(g_css, "example_panel1", &s);
	s.justify_content = QUI_JUSTIFY_FLEX_END;
	qui_stylesheet_add(g_css, "example_panel2", &s);
	s.flex_grow = 0.92f;
	qui_stylesheet_add(g_css, "example_panel3", &s);

	css_reset(&s);
	s.background_color = 0xAA000000;
	s.border_color = 0xFF00FFFF;
	s.border_width = 2;
	s.flex_grow = 0.3f;
	s.border_radius_bottom_left
		= s.border_radius_bottom_right
		= s.border_radius_top_left
		= s.border_radius_top_right = 20;
	s.box_shadow_blur = 10;
	s.box_shadow_color = 0x99000000;
	s.box_shadow_offset_x = 5;
	s.box_shadow_offset_y = 5;
	qui_stylesheet_add(g_css, "panel", &s);

	css_reset(&s);
	s.justify_content = QUI_JUSTIFY_CENTER;
	s.align_items = QUI_ALIGN_CENTER;
	s.flex_grow = 0.7f;
	s.display = QUI_DISPLAY_FLEX;
	qui_stylesheet_add(g_css, "menu_panel", &s);

	css_reset(&s);
	s.font_family_ref = font_ref;
	s.font_size = 2;
	s.color = 0xFFFFFFFF;
	s.padding_left = s.padding_right
		= s.padding_bottom = s.padding_top = 25;
	s.white_space = QUI_WS_PRE_WRAP;
	qui_stylesheet_add(g_css, "text", &s);

	s.color = 0xFFEEEEEE;
	s.background_color = 0;
	s.border_color = 0xFFFFFFFF;
	s.text_align = QUI_TEXT_ALIGN_CENTER;
	qui_stylesheet_add(g_css, "option", &s);

	s.border_color = s.color = 0xFFFFFFFF;
	s.background_color = 0x5500AAFF;
	s.border_width = 1;

	qui_stylesheet_add(g_css, "option-active", &s);

	css_reset(&s);
	s.background_color = 0x55000000;
	s.border_color = 0xFFFFFFFF;
	s.display = QUI_DISPLAY_NONE;
	qui_stylesheet_add(g_css, "input", &s);

	css_reset(&s);
	s.font_family_ref = font_ref;
	s.font_size = 2;
	s.color = 0xFFFFFFFF;
	s.position = QUI_POSITION_ABSOLUTE;
	s.display = QUI_DISPLAY_NONE;
	s.right = 10;
	s.bottom = 5;
	qui_stylesheet_add(g_css, "cursor", &s);
}

void dialog_init(void)
{
	unsigned qm_dialog = qmap_reg(sizeof(struct dialog));
	unsigned qm_opt = qmap_reg(sizeof(struct option));
	unsigned qm_input = qmap_reg(sizeof(struct input));

	dialog_hd = qmap_open(NULL, NULL, QM_HNDL, qm_dialog, 0xFF, QM_AINDEX);
	option_hd = qmap_open(NULL, NULL, QM_HNDL, qm_opt, 0xFF, QM_AINDEX);
	input_hd = qmap_open(NULL, NULL, QM_HNDL, qm_input, 0xFF, QM_AINDEX);

	dialog_seq = ids_init();
	qgl_size(&be_width, &be_height);

	dialog_build_styles();
	root = build_qui_tree();
	qui_apply_styles(root, g_css);
}

unsigned dialog_add(char *text)
{
	struct dialog d;

	memset(&d, 0, sizeof(d));
	d.text = text;
	d.options = ids_init();
	d.input = QM_MISS;
	return qmap_put(dialog_hd, NULL, &d);
}

static inline unsigned input_add(uint32_t cw, uint32_t ch, unsigned next, unsigned flags)
{
	struct input d;

	memset(&d, 0, sizeof(d));
	d.cw = cw;
	d.ch = ch;
	d.next = next;
	d.flags = flags;
	return qmap_put(input_hd, NULL, &d);
}

unsigned dialog_input(unsigned d_ref, uint32_t cw, uint32_t ch,
		      unsigned flags, char *next)
{
	struct dialog *d = (struct dialog *)
		qmap_get(dialog_hd, &d_ref);

	unsigned next_ref = dialog_add(next);
	unsigned input_ref = input_add(cw, ch, next_ref, flags);

	d->input = input_ref;
	return next_ref;
}

void dialog_then(unsigned ref, dialog_cb_t *cb)
{
	struct dialog *d = (struct dialog *)
		qmap_get(dialog_hd, &ref);

	d->then = cb;
}

static unsigned option_add(char *text, unsigned ref)
{
	struct option d = { .text = text, .ref = ref };

	return qmap_put(option_hd, NULL, &d);
}

unsigned dialog_option(unsigned ref, char *op_text, char *text)
{
	struct dialog *d = (struct dialog *)
		qmap_get(dialog_hd, &ref);

	unsigned new_ref = text ? dialog_add(text) : QM_MISS;
	unsigned new_o_ref = option_add(op_text, new_ref);

	ids_push(&d->options, new_o_ref);
	d->option_n++;
	return new_ref;
}

static void cursor_blink(void) {
	int new_blink_state = ((unsigned)round(time_tick * 2)) & 1;
	if (blink_state != new_blink_state) {
		QUI_STYLE(cursor, display, new_blink_state
			? QUI_DISPLAY_BLOCK : QUI_DISPLAY_NONE);
		blink_state = new_blink_state;
	}
}

void dialog_render(void)
{
	if (!cdialog.text)
		return;

	if (cursor_on)
		cursor_blink();

	qui_render(root);
}

void dialog_start(unsigned ref)
{
	if (cdialog.text)
		return;

	ids_drop(&dialog_seq);
	dialog_arg_n = 0;
	dialog_begin(ref);
}

int dialog_action(void)
{
	if (!cdialog.text)
		return 0;

	if (cdialog.input != QM_MISS) {
		struct input *in = (struct input *)
			qmap_get(input_hd, &cdialog.input);

		dialog_arg[dialog_arg_n++] = in->text;
		if (in->next != QM_MISS) {
			QUI_STYLE(input, display, QUI_DISPLAY_NONE);
			dialog_begin(in->next);
			return 1;
		}
	}

	if (cdialog.next || !cdialog.option_n) {
		cdialog.text = cdialog.next;
		if (!cdialog.text && cdialog.then)
			cdialog.then();
		qui_text(txt, cdialog.text);
		qui_rebuild();
		return 1;
	}

	idsi_t *it = ids_iter(&cdialog.options);
	unsigned ref;
	for (uint8_t i = 0; ids_next(&ref, &it) && i != cdialog.option; i++)
		;
	const struct option *opt = qmap_get(option_hd, &ref);

	cdialog.text = NULL;
	dialog_begin(opt->ref);
	return 0;
}

int dialog_showing(void)
{
	return !!cdialog.text;
}

int dialog_select(int down)
{
	if (!cdialog.option_n)
		return 0;

	if (down) {
		if (cdialog.option + 1 < cdialog.option_n) {
			cdialog.option++;
			update_options();
		}
	} else if (cdialog.option > 0) {
		cdialog.option--;
		update_options();
	}

	return 1;
}

int input_press(unsigned short code)
{
	if (!cdialog.text || cdialog.input == QM_MISS)
		return 0;

	struct input *in = (struct input *)
		qmap_get(input_hd, &cdialog.input);

	if (code == QGL_KEY_ENTER && !(in->flags & QGL_INPUT_MULTILINE)) {
		dialog_start(in->next);
		return 0;
	}

	in->len += qgl_key_parse(in->text, in->len, code, in->flags);

	qui_text(input, in->text);
	qui_rebuild();
	return 1;
}

char **dialog_args(void)
{
	return (char **)dialog_arg;
}
