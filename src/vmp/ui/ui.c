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
void ui_set_songs(vec_t* songs);
void ui_show_progress(char* progress);
void ui_update_palyer();
void ui_update_songlist();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "bm_rgba_util.c"
#include "coder.c"
#include "config.c"
#include "database.c"
#include "library.c"
#include "songlist.c"
#include "tg_css.c"
#include "tg_knob.c"
#include "tg_text.c"
#include "ui_compositor.c"
#include "ui_manager.c"
#include "ui_table.c"
#include "ui_util.c"
#include "vh_button.c"
#include "vh_key.c"
#include "vh_knob.c"
#include "viewer.c"
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
#include "zc_time.c"

struct _ui_t
{
    view_t* view_base;
    view_t* cursor; // replay cursor

    void* viewer;
    cb_t* sizecb;

    view_t* cover;
    view_t* visL;
    view_t* visR;

    ui_table_t* songlisttable;

    map_t* played_song;
    int    shuffle;

    view_t*     infotf;
    textstyle_t infots;
} ui;

void ui_on_key_down(void* userdata, void* data)
{
    // ev_t* ev = (ev_t*) data;
}

void ui_content_size_cb(void* userdata, void* data)
{
    /* v2_t* r = (v2_t*) data; */
    /* vh_cv_body_set_content_size(uiv.visubody, (int) r->x, (int) r->y); */
}

void ui_on_btn_event(void* userdata, void* data)
{
    // ev_t* ev = (ev_t*) data;
    view_t*      btnview = data;
    vh_button_t* vh      = btnview->handler_data;

    if (strcmp(btnview->id, "playbtn") == 0)
    {
	if (vh->state == VH_BUTTON_DOWN)
	{
	    if (ui.viewer) viewer_play(ui.viewer);
	    else
	    {
		map_t* song = songlist_get_song(0);
		if (song)
		{
		    if (ui.played_song) REL(ui.played_song);
		    ui.played_song = RET(ui.played_song);

		    char* path     = MGET(song, "path");
		    char* realpath = path_new_append(config_get("lib_path"), path);

		    ui.viewer = viewer_open(realpath, ui.sizecb);
		    // TODO update songlisttable
		}
	    }
	}
	else
	{
	    if (ui.viewer) viewer_pause(ui.viewer);
	}
    };
    if (strcmp(btnview->id, "mutebtn") == 0)
    {
	if (ui.viewer)
	{
	    if (vh->state == VH_BUTTON_DOWN) viewer_mute(ui.viewer);
	    else viewer_unmute(ui.viewer);
	}
    };
    if (strcmp(btnview->id, "prevbtn") == 0)
    {
	if (ui.viewer) viewer_close(ui.viewer);
	ui.viewer = NULL;

	map_t* prev = NULL;

	if (ui.shuffle == 0 && ui.played_song) prev = songlist_get_prev_song(ui.played_song);
	else prev = songlist_get_song(1);

	if (prev)
	{
	    if (ui.played_song) REL(ui.played_song);
	    ui.played_song = RET(prev);

	    char* path     = MGET(prev, "path");
	    char* realpath = path_new_append(config_get("lib_path"), path);

	    ui.viewer = viewer_open(realpath, ui.sizecb);
	    // TODO update songlisttable
	}
    };
    if (strcmp(btnview->id, "nextbtn") == 0)
    {
	if (ui.viewer) viewer_close(ui.viewer);
	ui.viewer = NULL;

	map_t* next = NULL;

	if (ui.shuffle == 0 && ui.played_song) next = songlist_get_prev_song(ui.played_song);
	else next = songlist_get_song(1);

	if (next)
	{
	    if (ui.played_song) REL(ui.played_song);
	    ui.played_song = RET(next);

	    char* path     = MGET(next, "path");
	    char* realpath = path_new_append(config_get("lib_path"), path);

	    ui.viewer = viewer_open(realpath, ui.sizecb);
	    // TODO update songlisttable
	}
    };
    if (strcmp(btnview->id, "shufflebtn") == 0)
    {
	ui.shuffle = 1 - ui.shuffle;
    };
    if (strcmp(btnview->id, "settingsbtn") == 0)
    {
	// show settings popup
    };
    if (strcmp(btnview->id, "donatebtn") == 0)
    {
	// show donate popup
    };
    if (strcmp(btnview->id, "filterbtn") == 0)
    {
	// show filter popup
    };
    if (strcmp(btnview->id, "clearbtn") == 0)
    {
	// clear filter bar
    };
    if (strcmp(btnview->id, "exitbtn") == 0) wm_close();
    if (strcmp(btnview->id, "maxbtn") == 0) wm_toggle_fullscreen();
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
	    vec_t* selected = userdata;
	    map_t* info     = selected->data[0];

	    if (ui.played_song) REL(ui.played_song);
	    ui.played_song = RET(info);

	    char* path     = MGET(info, "path");
	    char* realpath = path_new_append(config_get("lib_path"), path);

	    if (ui.viewer) viewer_close(ui.viewer);
	    ui.viewer = viewer_open(realpath, ui.sizecb);

	    view_t* playbtn = view_get_subview(ui.view_base, "playbtn");
	    vh_button_set_state(playbtn, VH_BUTTON_DOWN);

	    REL(realpath);
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
	    ev_t ev = *((ev_t*) userdata);

	    if (ev.keycode == SDLK_DOWN || ev.keycode == SDLK_UP)
	    {
		int32_t index = table->selected_index;

		if (ev.keycode == SDLK_DOWN) index += 1;
		if (ev.keycode == SDLK_UP) index -= 1;
		ui_table_select(table, index);
	    }
	}
	break;
	case UI_TABLE_EVENT_DROP:
	    zc_log_debug("drop %s", table->id);

	    break;
    }
}

void ui_pos_change(view_t* view, float angle)
{
}

void ui_vol_change(view_t* view, float angle)
{
}

void ui_create_views(float width, float height)
{
    // generate views from descriptors

    vec_t* view_list = viewgen_html_create(config_get("html_path"));
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

    /* size callback */

    cb_t* sizecb = cb_new(ui_content_size_cb, NULL);

    ui.sizecb = sizecb;

    /* app buttons */

    cb_t* btn_cb = cb_new(ui_on_btn_event, NULL);

    /* knobs */

    view_t* seekknob = view_get_subview(ui.view_base, "seekknob");
    view_t* playbtn  = view_get_subview(ui.view_base, "playbtn");
    view_t* volknob  = view_get_subview(ui.view_base, "volknob");
    view_t* mutebtn  = view_get_subview(ui.view_base, "mutebtn");

    tg_knob_add(seekknob);
    vh_knob_add(seekknob, ui_pos_change);

    tg_knob_add(volknob);
    vh_knob_add(volknob, ui_vol_change);

    vh_button_add(playbtn, VH_BUTTON_TOGGLE, btn_cb);
    vh_button_add(mutebtn, VH_BUTTON_TOGGLE, btn_cb);

    REL(btn_cb);

    /* buttons */

    view_t* prevbtn     = view_get_subview(ui.view_base, "prevbtn");
    view_t* nextbtn     = view_get_subview(ui.view_base, "nextbtn");
    view_t* shufflebtn  = view_get_subview(ui.view_base, "shufflebtn");
    view_t* editbtn     = view_get_subview(ui.view_base, "editbtn");
    view_t* settingsbtn = view_get_subview(ui.view_base, "settingsbtn");
    view_t* donatebtn   = view_get_subview(ui.view_base, "donatebtn");
    view_t* maxbtn      = view_get_subview(ui.view_base, "maxbtn");
    view_t* exitbtn     = view_get_subview(ui.view_base, "exitbtn");
    view_t* filterbtn   = view_get_subview(ui.view_base, "filterbtn");
    view_t* clearbtn    = view_get_subview(ui.view_base, "clearbtn");

    vh_button_add(prevbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(nextbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(shufflebtn, VH_BUTTON_TOGGLE, btn_cb);
    vh_button_add(editbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(settingsbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(donatebtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(maxbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(exitbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(filterbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(clearbtn, VH_BUTTON_NORMAL, btn_cb);

    /* textfields */

    ui.infotf = view_get_subview(ui.view_base, "infotf");
    ui.infots = ui_util_gen_textstyle(ui.infotf);

    tg_text_add(ui.infotf);
    tg_text_set(ui.infotf, "This is the info textfield", ui.infots);

    view_t* filtertf = view_get_subview(ui.view_base, "filtertf");

    textstyle_t filterts = ui_util_gen_textstyle(filtertf);

    tg_text_add(filtertf);
    tg_text_set(filtertf, "This is the search textfield", filterts);

    view_t* timetf = view_get_subview(ui.view_base, "timetf");

    textstyle_t timets = ui_util_gen_textstyle(timetf);

    tg_text_add(timetf);
    tg_text_set(timetf, "05:23 / 08:33", timets);

    /* songlist */

    vec_t* fields = VNEW();

    VADDR(fields, cstr_new_cstring("artist"));
    VADDR(fields, num_new_int(200));
    VADDR(fields, cstr_new_cstring("album"));
    VADDR(fields, num_new_int(200));
    VADDR(fields, cstr_new_cstring("title"));
    VADDR(fields, num_new_int(350));
    VADDR(fields, cstr_new_cstring("date"));
    VADDR(fields, num_new_int(70));
    VADDR(fields, cstr_new_cstring("genre"));
    VADDR(fields, num_new_int(150));
    VADDR(fields, cstr_new_cstring("track"));
    VADDR(fields, num_new_int(60));
    VADDR(fields, cstr_new_cstring("disc"));
    VADDR(fields, num_new_int(60));
    VADDR(fields, cstr_new_cstring("duration"));
    VADDR(fields, num_new_int(50));
    VADDR(fields, cstr_new_cstring("channels"));
    VADDR(fields, num_new_int(40));
    VADDR(fields, cstr_new_cstring("bitrate"));
    VADDR(fields, num_new_int(100));
    VADDR(fields, cstr_new_cstring("samplerate"));
    VADDR(fields, num_new_int(80));
    VADDR(fields, cstr_new_cstring("plays"));
    VADDR(fields, num_new_int(55));
    VADDR(fields, cstr_new_cstring("skips"));
    VADDR(fields, num_new_int(55));
    VADDR(fields, cstr_new_cstring("added"));
    VADDR(fields, num_new_int(150));
    VADDR(fields, cstr_new_cstring("played"));
    VADDR(fields, num_new_int(155));
    VADDR(fields, cstr_new_cstring("skipped"));
    VADDR(fields, num_new_int(155));
    VADDR(fields, cstr_new_cstring("type"));
    VADDR(fields, num_new_int(80));
    VADDR(fields, cstr_new_cstring("container"));
    VADDR(fields, num_new_int(80));

    view_t* songlist       = view_get_subview(ui.view_base, "songlisttable");
    view_t* songlistscroll = view_get_subview(ui.view_base, "songlistscroll");
    view_t* songlistevt    = view_get_subview(ui.view_base, "songlistevt");
    view_t* songlisthead   = view_get_subview(ui.view_base, "songlisthead");

    /* textstyle_t ts = ui_util_gen_textstyle(songlist); */

    /* ts.align        = TA_CENTER; */
    /* ts.margin_right = 0; */
    /* ts.size         = 60.0; */
    /* ts.textcolor    = 0x353535FF; */
    /* ts.backcolor    = 0x252525FF; */
    /* ts.multiline    = 0; */

    ui.songlisttable = ui_table_create(
	"songlisttable",
	songlist,
	songlistscroll,
	songlistevt,
	songlisthead,
	fields,
	on_songlist_event);

    REL(fields);

    ui_manager_activate(songlistevt);

    /* get visual views */

    ui.cover = view_get_subview(ui.view_base, "cover");
    ui.visL  = view_get_subview(ui.view_base, "visL");
    ui.visR  = view_get_subview(ui.view_base, "visR");

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

    if (ui.played_song) REL(ui.played_song);

    REL(ui.sizecb);
    REL(ui.view_base);
    REL(ui.songlisttable);

    ui_manager_destroy(); // DESTROY 1

    text_destroy(); // DESTROY 0
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

void ui_update_palyer()
{
    if (ui.viewer)
    {
	double rem;
	viewer_video_refresh(ui.viewer, &rem, ui.cover->texture.bitmap);
	viewer_audio_refresh(ui.viewer, ui.visL->texture.bitmap, ui.visR->texture.bitmap);
	ui.cover->texture.changed = 1;
	ui.visL->texture.changed  = 1;
	ui.visR->texture.changed  = 1;
    }
}

void ui_update_layout(float w, float h)
{
    /* view_layout(ui.view_base); */
}

void ui_describe()
{
    view_describe(ui.view_base, 0);
}

void ui_set_songs(vec_t* songs)
{
    ui_table_set_data(ui.songlisttable, songs);
}

void ui_show_progress(char* progress)
{
    tg_text_set(ui.infotf, progress, ui.infots);
}

void ui_update_songlist()
{
    vec_t* entries = VNEW();
    db_get_entries(entries);
    songlist_set_songs(entries);
    ui_set_songs(songlist_get_visible_songs());
}

#endif
