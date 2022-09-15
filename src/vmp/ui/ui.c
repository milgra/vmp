#ifndef ui_h
#define ui_h

#include "view.c"

void ui_init(float width, float height);
void ui_destroy();
void ui_add_cursor();
void ui_update_cursor(r2_t frame);
void ui_render_without_cursor(uint32_t time);
void ui_save_screenshot(uint32_t time, char hide_cursor);
void ui_update_layout(float w, float h);
void ui_describe();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "bm_rgba_util.c"
#include "coder.c"
#include "config.c"
#include "tg_css.c"
#include "tg_text.c"
#include "ui_compositor.c"
#include "ui_manager.c"
#include "ui_table.c"
#include "ui_util.c"
#include "vh_button.c"
#include "vh_key.c"
#include "viewgen_css.c"
#include "viewgen_html.c"
#include "viewgen_type.c"
#include "wm_connector.c"
#include "zc_bm_rgba.c"
#include "zc_callback.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_number.c"
#include "zc_path.c"
#include "zc_text.c"

struct _ui_t
{
    view_t* view_base;
    view_t* cursor; // replay cursor

    view_t* exit_btn;
    view_t* full_btn;

    ui_table_t* songlisttable;
} ui;

void ui_on_key_down(void* userdata, void* data)
{
    // ev_t* ev = (ev_t*) data;
}

void ui_on_btn_event(void* userdata, void* data)
{
    // ev_t* ev = (ev_t*) data;
    view_t* btnview = data;

    if (btnview == ui.exit_btn) wm_close();
    if (btnview == ui.full_btn) wm_toggle_fullscreen();
}

void on_songlist_event(ui_table_t* table, ui_table_event event, void* userdata)
{
    switch (event)
    {
	case UI_TABLE_EVENT_FIELDS_UPDATE:
	{
	    zc_log_debug("fields update %s", table->id);
	}
	break;
	case UI_TABLE_EVENT_SELECT:
	{
	    zc_log_debug("select %s", table->id);
	}
	break;
	case UI_TABLE_EVENT_OPEN:
	{
	    zc_log_debug("open %s", table->id);
	}
	break;
	case UI_TABLE_EVENT_DRAG:
	{
	    zc_log_debug("drag %s", table->id);
	}
	break;
	case UI_TABLE_EVENT_KEY:
	{
	    zc_log_debug("key %s", table->id);
	}
	break;
	case UI_TABLE_EVENT_DROP:
	    zc_log_debug("drop %s", table->id);

	    break;
    }
}

void ui_add_cursor()
{
    ui.cursor                         = view_new("ui.cursor", ((r2_t){10, 10, 10, 10}));
    ui.cursor->exclude                = 0;
    ui.cursor->style.background_color = 0xFF000099;
    ui.cursor->needs_touch            = 0;
    tg_css_add(ui.cursor);
    ui_manager_add_to_top(ui.cursor);
}

void ui_update_cursor(r2_t frame)
{
    view_set_frame(ui.cursor, frame);
}

void ui_render_without_cursor(uint32_t time)
{
    ui_manager_remove(ui.cursor);
    ui_manager_render(time);
    ui_manager_add_to_top(ui.cursor);
}

void ui_save_screenshot(uint32_t time, char hide_cursor)
{
    if (config_get("lib_path"))
    {
	static int cnt    = 0;
	view_t*    root   = ui_manager_get_root();
	r2_t       frame  = root->frame.local;
	bm_rgba_t* screen = bm_rgba_new(frame.w, frame.h); // REL 0

	// remove cursor for screenshot to remain identical

	if (hide_cursor) ui_render_without_cursor(time);

	ui_compositor_render_to_bmp(screen);

	char*      name    = cstr_new_format(20, "screenshot%.3i.png", cnt++); // REL 1
	char*      path    = path_new_append(config_get("lib_path"), name);    // REL 2
	bm_rgba_t* flipped = bm_rgba_new_flip_y(screen);                       // REL 3

	coder_write_png(path, flipped);

	REL(flipped); // REL 3
	REL(name);    // REL 2
	REL(path);    // REL 1
	REL(screen);  // REL 0

	if (hide_cursor) ui_update_cursor(frame); // full screen cursor to indicate screenshot, next step will reset it
    }
}

void ui_create_views(float width, float height)
{
    // generate views from descriptors

    vec_t* view_list = viewgen_html_create(config_get("html_path"));

    zc_log_debug("VIEW LIST");
    mem_describe(view_list, 0);

    viewgen_css_apply(view_list, config_get("css_path"), config_get("res_path"));
    viewgen_type_apply(view_list);
    ui.view_base = RET(vec_head(view_list));
    REL(view_list);

    // initial layout of views

    view_set_frame(ui.view_base, (r2_t){0.0, 0.0, (float) width, (float) height});
    ui_manager_add(ui.view_base);
}

int ui_comp_text(void* left, void* right)
{
    char* la = left;
    char* ra = right;

    return strcmp(la, ra);
}

void ui_init(float width, float height)
{
    text_init();                    // DESTROY 0
    ui_manager_init(width, height); // DESTROY 1
    ui_create_views(width, height);

    // setup key events

    cb_t* key_cb = cb_new(ui_on_key_down, NULL);
    vh_key_add(ui.view_base, key_cb); // listen on ui.view_base for shortcuts
    REL(key_cb);

    /* app buttons */

    cb_t* btn_cb = cb_new(ui_on_btn_event, NULL);

    ui.exit_btn = view_get_subview(ui.view_base, "app_close_btn");
    vh_button_add(ui.exit_btn, VH_BUTTON_NORMAL, btn_cb);

    ui.full_btn = view_get_subview(ui.view_base, "app_maximize_btn");
    vh_button_add(ui.full_btn, VH_BUTTON_NORMAL, btn_cb);

    /* songlist */

    vec_t* fields = VNEW();
    VADDR(fields, cstr_new_cstring("file/basename"));
    VADDR(fields, num_new_int(400));
    /* VADDR(fields, cstr_new_cstring("file/mime")); */
    /* VADDR(fields, num_new_int(200)); */
    /* VADDR(fields, cstr_new_cstring("file/path")); */
    /* VADDR(fields, num_new_int(200)); */
    VADDR(fields, cstr_new_cstring("file/size"));
    VADDR(fields, num_new_int(120));
    VADDR(fields, cstr_new_cstring("file/last_access"));
    VADDR(fields, num_new_int(180));
    VADDR(fields, cstr_new_cstring("file/last_modification"));
    VADDR(fields, num_new_int(180));
    VADDR(fields, cstr_new_cstring("file/last_status"));
    VADDR(fields, num_new_int(180));

    view_t* songlist       = view_get_subview(ui.view_base, "songlisttable");
    view_t* songlistscroll = view_get_subview(ui.view_base, "songlistscroll");
    view_t* songlistevt    = view_get_subview(ui.view_base, "songlistevt");
    view_t* songlisthead   = view_get_subview(ui.view_base, "songlisthead");

    textstyle_t ts = ui_util_gen_textstyle(songlist);

    ts.align        = TA_CENTER;
    ts.margin_right = 0;
    ts.size         = 60.0;
    ts.textcolor    = 0x353535FF;
    ts.backcolor    = 0x252525FF;
    ts.multiline    = 0;

    if (songlist)
    {
	tg_text_add(songlist);
	tg_text_set(songlist, "SONGS", ts);
    }
    else
	zc_log_debug("songlistbck not found");

    ui.songlisttable = ui_table_create(
	"songlisttable",
	songlist,
	songlistscroll,
	songlistevt,
	songlisthead,
	fields,
	on_songlist_event);

    /* ui.song_list_data = VNEW(); // REL S0 */
    /* ui_table_set_data(ui.songlisttable, ui.song_list_data); */

    ui_manager_activate(songlistevt);

    // show texture map for debug

    /* view_t* texmap       = view_new("texmap", ((r2_t){0, 0, 150, 150})); */
    /* texmap->needs_touch  = 0; */
    /* texmap->exclude      = 0; */
    /* texmap->texture.full = 1; */
    /* texmap->style.right  = 0; */
    /* texmap->style.top    = 0; */

    /* ui_manager_add(texmap); */
}

void ui_destroy()
{
    ui_manager_remove(ui.view_base);

    REL(ui.view_base);

    ui_manager_destroy(); // DESTROY 1

    text_destroy(); // DESTROY 0
}

void ui_update_layout(float w, float h)
{
    /* view_layout(ui.view_base); */
}

void ui_describe()
{
    view_describe(ui.view_base, 0);
}

#endif
