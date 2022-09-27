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
void ui_play_next();
void ui_play_prev();
void ui_toggle_pause();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "bm_rgba_util.c"
#include "coder.c"
#include "config.c"
#include "library.c"
#include "map_util.c"
#include "mediaplayer.c"
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
#include "vh_textinput.c"
#include "vh_touch.c"
#include "view_layout.c"
#include "viewgen_css.c"
#include "viewgen_html.c"
#include "viewgen_type.c"
#include "wm_connector.c"
#include "zc_bm_rgba.c"
#include "zc_callback.c"
#include "zc_cstring.c"
#include "zc_draw.c"
#include "zc_log.c"
#include "zc_number.c"
#include "zc_path.c"
#include "zc_text.c"
#include "zc_time.c"

struct _ui_t
{
    MediaState* ms;

    ui_table_t* metalisttable;
    ui_table_t* songlisttable;
    ui_table_t* genrelisttable;
    ui_table_t* artistlisttable;
    ui_table_t* settingslisttable;

    view_t* metapopupcont;
    view_t* filterpopupcont;
    view_t* settingspopupcont;

    cb_t* sizecb;

    view_t* cursor;
    view_t* view_base;

    view_t* visL;
    view_t* visR;
    view_t* cover;
    view_t* visuals;
    view_t* songlisttop;

    view_t* volknob;
    view_t* seekknob;

    view_t* infotf;
    view_t* timetf;
    view_t* filtertf;

    textstyle_t infots;
    textstyle_t timets;
    textstyle_t filterts;

    map_t* played_song;
    int    shuffle;
    float  volume;
    int    visutype; // 0 - waves 1 - rdft
    int    hide_visuals;
    float  timestate;
} ui;

void ui_play_song(map_t* song)
{
    /* close existing ms */
    if (ui.ms) mp_close(ui.ms);
    ui.ms = NULL;

    /* update played song */
    if (ui.played_song) REL(ui.played_song);
    ui.played_song = RET(song);

    /* create new ms */
    char* path     = MGET(song, "path");
    char* realpath = path_new_append(config_get("lib_path"), path);

    ui.ms = mp_open(realpath, ui.sizecb);
    mp_set_volume(ui.ms, ui.volume);
    mp_set_visutype(ui.ms, ui.visutype);

    /* clear cover view */
    gfx_rect(ui.cover->texture.bitmap, 0, 0, ui.cover->texture.bitmap->w, ui.cover->texture.bitmap->h, 0x151515FF, 1);

    /* update song list table */
    uint32_t index = songlist_get_index(song);
    if (index < UINT32_MAX) ui_table_select(ui.songlisttable, index);

    /* update info text */
    char info[200];
    snprintf(info, 200, "%s / %s", (char*) MGET(song, "artist"), (char*) MGET(song, "title"));
    tg_text_set(ui.infotf, info, ui.infots);

    view_t* playbtn = view_get_subview(ui.view_base, "playbtn");
    if (playbtn) vh_button_set_state(playbtn, VH_BUTTON_DOWN);
}

void ui_play_next()
{
    map_t* next = NULL;

    if (ui.shuffle == 0 && ui.played_song) next = songlist_get_next_song(ui.played_song);
    else next = songlist_get_song(1);

    ui_play_song(next);
}

void ui_play_prev()
{
    map_t* prev = NULL;

    if (ui.shuffle == 0 && ui.played_song) prev = songlist_get_prev_song(ui.played_song);
    else prev = songlist_get_song(1);

    ui_play_song(prev);
}

void ui_toggle_pause()
{
    if (ui.ms)
    {
	view_t* playbtn = view_get_subview(ui.view_base, "playbtn");

	if (!ui.ms->paused)
	{
	    mp_pause(ui.ms);
	    if (playbtn) vh_button_set_state(playbtn, VH_BUTTON_UP);
	}
	else
	{
	    mp_play(ui.ms);
	    if (playbtn) vh_button_set_state(playbtn, VH_BUTTON_DOWN);
	}
    }
}

void ui_on_key_down(void* userdata, void* data)
{
    ev_t* ev = (ev_t*) data;
    if (ev->keycode == SDLK_SPACE) ui_toggle_pause();
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
	    if (ui.ms) mp_play(ui.ms);
	    else
	    {
		map_t* song = songlist_get_song(0);
		if (song) ui_play_song(song);
	    }
	}
	else
	{
	    if (ui.ms) mp_pause(ui.ms);
	}
    };
    if (strcmp(btnview->id, "mutebtn") == 0)
    {
	if (ui.ms)
	{
	    if (vh->state == VH_BUTTON_DOWN) mp_mute(ui.ms);
	    else mp_unmute(ui.ms);
	}
    };
    if (strcmp(btnview->id, "prevbtn") == 0)
    {
	map_t* prev = NULL;

	if (ui.shuffle == 0 && ui.played_song) prev = songlist_get_prev_song(ui.played_song);
	else prev = songlist_get_song(1);

	if (prev) ui_play_song(prev);
    };
    if (strcmp(btnview->id, "nextbtn") == 0)
    {
	map_t* next = NULL;

	if (ui.shuffle == 0 && ui.played_song) next = songlist_get_next_song(ui.played_song);
	else next = songlist_get_song(1);

	if (next) ui_play_song(next);
    };
    if (strcmp(btnview->id, "shufflebtn") == 0)
    {
	ui.shuffle = 1 - ui.shuffle;
    };
    if (strcmp(btnview->id, "settingsbtn") == 0)
    {
	if (!ui.settingspopupcont->parent)
	{
	    view_add_subview(ui.view_base, ui.settingspopupcont);
	    view_layout(ui.view_base);
	}
    };
    if (strcmp(btnview->id, "visubtn") == 0)
    {
	ui.hide_visuals = 1 - ui.hide_visuals;

	view_t* songs = view_get_subview(ui.view_base, "songs");

	if (ui.hide_visuals)
	{
	    view_remove_from_parent(ui.visuals);
	    view_remove_from_parent(ui.songlisttop);
	}
	else
	{
	    view_insert_subview(songs, ui.songlisttop, 0);
	    view_add_subview(ui.view_base, ui.visuals);
	}
	view_layout(ui.view_base);
    };
    if (strcmp(btnview->id, "editbtn") == 0)
    {
	// show filter popup
	if (!ui.metapopupcont->parent)
	{
	    view_add_subview(ui.view_base, ui.metapopupcont);

	    if (ui.songlisttable->selected->length > 0)
	    {
		map_t* info = ui.songlisttable->selected->data[0];

		view_t* cover    = view_get_subview(ui.metapopupcont, "metacover");
		char*   path     = MGET(info, "path");
		char*   realpath = path_new_append(config_get("lib_path"), path);

		if (!cover->texture.bitmap) view_gen_texture(cover);

		coder_load_cover_into(realpath, cover->texture.bitmap);
		REL(realpath);
		cover->texture.changed = 1;

		vec_t* pairs = VNEW();
		vec_t* keys  = VNEW();
		map_keys(info, keys);
		for (int index = 0; index < keys->length; index++)
		{
		    char*  key   = keys->data[index];
		    char*  value = MGET(info, key);
		    map_t* map   = MNEW();
		    MPUT(map, "key", key);
		    MPUT(map, "value", value);
		    VADDR(pairs, map);
		}
		ui_table_set_data(ui.metalisttable, pairs);
		REL(pairs);

		view_layout(ui.view_base);
	    }

	    /* vec_t* genres  = VNEW(); */
	    /* vec_t* artists = VNEW(); */

	    /* lib_get_genres(genres); */
	    /* lib_get_artists(artists); */

	    /* ui_table_set_data(ui.genrelisttable, genres); */
	    /* ui_table_set_data(ui.artistlisttable, artists); */

	    /* REL(genres); */
	    /* REL(artists); */
	}
    };
    if (strcmp(btnview->id, "filterbtn") == 0)
    {
	// show filter popup
	if (!ui.filterpopupcont->parent)
	{
	    view_add_subview(ui.view_base, ui.filterpopupcont);

	    vec_t* genres  = VNEW();
	    vec_t* artists = VNEW();

	    lib_get_genres(genres);
	    lib_get_artists(artists);

	    ui_table_set_data(ui.genrelisttable, genres);
	    ui_table_set_data(ui.artistlisttable, artists);

	    REL(genres);
	    REL(artists);

	    view_layout(ui.view_base);
	}
    };
    if (strcmp(btnview->id, "clearbtn") == 0)
    {
	vh_textinput_set_text(ui.filtertf, "");
	ui_manager_activate(ui.filtertf);
	vh_textinput_activate(ui.filtertf, 1);

	songlist_set_filter(NULL);
	ui_update_songlist();
    };
    if (strcmp(btnview->id, "exitbtn") == 0) wm_close();
    if (strcmp(btnview->id, "maxbtn") == 0) wm_toggle_fullscreen();
    if (strcmp(btnview->id, "visL") == 0)
    {
	ui.visutype = 1 - ui.visutype;
	if (ui.ms) mp_set_visutype(ui.ms, ui.visutype);
    }
    if (strcmp(btnview->id, "visR") == 0)
    {
	ui.visutype = 1 - ui.visutype;
	if (ui.ms) mp_set_visutype(ui.ms, ui.visutype);
    }
    if (strcmp(btnview->id, "settingsclosebtn") == 0)
    {
	view_remove_from_parent(ui.settingspopupcont);
    }
    if (strcmp(btnview->id, "filterclosebtn") == 0)
    {
	view_remove_from_parent(ui.filterpopupcont);
    }
    if (strcmp(btnview->id, "metaclosebtn") == 0)
    {
	view_remove_from_parent(ui.metapopupcont);
    }
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

	    ui_play_song(info);
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

void on_settingslist_event(ui_table_t* table, ui_table_event event, void* userdata)
{
    switch (event)
    {
	case UI_TABLE_EVENT_SELECT:
	{
	    zc_log_debug("select %s", table->id);
	}
	break;
    }
}

void on_genrelist_event(ui_table_t* table, ui_table_event event, void* userdata)
{
    switch (event)
    {
	case UI_TABLE_EVENT_SELECT:
	{
	    vec_t* selected = userdata;
	    map_t* info     = selected->data[0];

	    char* genre = MGET(info, "genre");
	    char  filter[100];

	    snprintf(filter, 100, "genre is %s", genre);
	    songlist_set_filter(filter);
	    ui_update_songlist();

	    vh_textinput_set_text(ui.filtertf, filter);
	}
	break;
    }
}

void on_artistlist_event(ui_table_t* table, ui_table_event event, void* userdata)
{
    switch (event)
    {
	case UI_TABLE_EVENT_SELECT:
	{
	    vec_t* selected = userdata;
	    map_t* info     = selected->data[0];

	    char* artist = MGET(info, "artist");
	    char  filter[100];

	    snprintf(filter, 100, "artist is %s", artist);
	    songlist_set_filter(filter);
	    ui_update_songlist();

	    vh_textinput_set_text(ui.filtertf, filter);
	}
	break;
    }
}

void on_metalist_event(ui_table_t* table, ui_table_event event, void* userdata)
{
    switch (event)
    {
	case UI_TABLE_EVENT_SELECT:
	{
	}
	break;
    }
}

void ui_on_filter(view_t* view, void* userdata)
{
    str_t* text   = vh_textinput_get_text(view);
    char*  filter = str_new_cstring(text); // REL 0

    songlist_set_filter(filter);
    ui_update_songlist();

    REL(filter); // REL 0
}

void ui_pos_change(view_t* view, float angle)
{
    float ratio = 0.0;
    if (angle > 0 && angle < 3.14 * 3 / 2)
    {
	ratio = angle / 6.28 + 0.25;
    }
    else if (angle > 3.14 * 3 / 2)
    {
	ratio = (angle - (3.14 * 3 / 2)) / 6.28;
    }

    ui.volume = ratio;
    mp_set_position(ui.ms, ui.volume);
}

void ui_vol_change(view_t* view, float angle)
{
    float ratio = 0.0;
    if (angle > 0 && angle < 3.14 * 3 / 2)
    {
	ratio = angle / 6.28 + 0.25;
    }
    else if (angle > 3.14 * 3 / 2)
    {
	ratio = (angle - (3.14 * 3 / 2)) / 6.28;
    }

    ui.volume = ratio;
    mp_set_volume(ui.ms, ui.volume);
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

    /* setup key events */

    cb_t* key_cb = cb_new(ui_on_key_down, NULL);
    vh_key_add(ui.view_base, key_cb); // listen on ui.view_base for shortcuts
    REL(key_cb);
    ui.view_base->needs_key = 1;

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

    ui.seekknob = seekknob;
    ui.volknob  = volknob;

    /* setup volume */

    ui.volume = 0.8;
    tg_knob_set_angle(ui.volknob, ui.volume * 6.28 - 3.14 / 2.0);

    /* buttons */

    view_t* prevbtn          = view_get_subview(ui.view_base, "prevbtn");
    view_t* nextbtn          = view_get_subview(ui.view_base, "nextbtn");
    view_t* shufflebtn       = view_get_subview(ui.view_base, "shufflebtn");
    view_t* editbtn          = view_get_subview(ui.view_base, "editbtn");
    view_t* settingsbtn      = view_get_subview(ui.view_base, "settingsbtn");
    view_t* visubtn          = view_get_subview(ui.view_base, "visubtn");
    view_t* maxbtn           = view_get_subview(ui.view_base, "maxbtn");
    view_t* exitbtn          = view_get_subview(ui.view_base, "exitbtn");
    view_t* filterbtn        = view_get_subview(ui.view_base, "filterbtn");
    view_t* clearbtn         = view_get_subview(ui.view_base, "clearbtn");
    view_t* filterclosebtn   = view_get_subview(ui.view_base, "filterclosebtn");
    view_t* settingsclosebtn = view_get_subview(ui.view_base, "settingsclosebtn");
    view_t* metaclosebtn     = view_get_subview(ui.view_base, "metaclosebtn");

    vh_button_add(prevbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(nextbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(shufflebtn, VH_BUTTON_TOGGLE, btn_cb);
    vh_button_add(editbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(settingsbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(visubtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(maxbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(exitbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(filterbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(clearbtn, VH_BUTTON_NORMAL, btn_cb);

    vh_button_add(metaclosebtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(filterclosebtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(settingsclosebtn, VH_BUTTON_NORMAL, btn_cb);

    /* textfields */

    ui.infotf = view_get_subview(ui.view_base, "infotf");
    ui.infots = ui_util_gen_textstyle(ui.infotf);

    tg_text_add(ui.infotf);
    tg_text_set(ui.infotf, "Ready", ui.infots);

    ui.filtertf = view_get_subview(ui.view_base, "filtertf");
    ui.filterts = ui_util_gen_textstyle(ui.filtertf);

    vh_textinput_add(ui.filtertf, "", "Filter", ui.filterts, NULL);
    vh_textinput_set_on_text(ui.filtertf, ui_on_filter);

    ui.timetf = view_get_subview(ui.view_base, "timetf");
    ui.timets = ui_util_gen_textstyle(ui.timetf);

    tg_text_add(ui.timetf);
    tg_text_set(ui.timetf, "00:00 / 08:00", ui.timets);

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

    /* settings list */

    view_t* settingspopupcont = view_get_subview(ui.view_base, "settingspopupcont");
    view_t* settingspopup     = view_get_subview(ui.view_base, "settingspopup");
    view_t* settingslist      = view_get_subview(ui.view_base, "settingslisttable");
    view_t* settingslistevt   = view_get_subview(ui.view_base, "settingslistevt");

    settingspopup->blocks_touch  = 1;
    settingspopup->blocks_scroll = 1;

    ui.settingspopupcont = RET(settingspopupcont);

    fields = VNEW();

    VADDR(fields, cstr_new_cstring("value"));
    VADDR(fields, num_new_int(510));

    ui.settingslisttable = ui_table_create(
	"settingslisttable",
	settingslist,
	NULL,
	settingslistevt,
	NULL,
	fields,
	on_settingslist_event);

    REL(fields);

    vec_t* items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Visual Music Player v%s beta by Milan Toth", VMP_VERSION)}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Free and Open Source Software")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Donate on Paypal")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Organize Library : %s", config_get("lib_organize"))}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Library Path : %s", config_get("lib_path"))}));

    ui_table_set_data(ui.settingslisttable, items);
    REL(items);

    view_remove_from_parent(ui.settingspopupcont);

    /* genre and artist lists */

    view_t* filterpopupcont = view_get_subview(ui.view_base, "filterpopupcont");
    view_t* filterpopup     = view_get_subview(ui.view_base, "filterpopup");
    view_t* genrelist       = view_get_subview(ui.view_base, "genrelisttable");
    view_t* genrelistevt    = view_get_subview(ui.view_base, "genrelistevt");
    view_t* artistlist      = view_get_subview(ui.view_base, "artistlisttable");
    view_t* artistlistevt   = view_get_subview(ui.view_base, "artistlistevt");

    filterpopup->blocks_touch  = 1;
    filterpopup->blocks_scroll = 1;

    ui.filterpopupcont = RET(filterpopupcont);

    vec_t* genrefields  = VNEW();
    vec_t* artistfields = VNEW();

    VADDR(genrefields, cstr_new_cstring("genre"));
    VADDR(genrefields, num_new_int(150));

    VADDR(artistfields, cstr_new_cstring("artist"));
    VADDR(artistfields, num_new_int(350));

    ui.genrelisttable = ui_table_create(
	"genrelisttable",
	genrelist,
	NULL,
	genrelistevt,
	NULL,
	genrefields,
	on_genrelist_event);

    ui.artistlisttable = ui_table_create(
	"artistlisttable",
	artistlist,
	NULL,
	artistlistevt,
	NULL,
	artistfields,
	on_artistlist_event);

    REL(genrefields);
    REL(artistfields);

    view_remove_from_parent(ui.filterpopupcont);

    /* song metadata */

    view_t* metapopupcont = view_get_subview(ui.view_base, "metapopupcont");
    view_t* metapopup     = view_get_subview(ui.view_base, "metapopup");
    view_t* metalist      = view_get_subview(ui.view_base, "metalisttable");
    view_t* metalistevt   = view_get_subview(ui.view_base, "metalistevt");

    metapopup->blocks_touch  = 1;
    metapopup->blocks_scroll = 1;

    ui.metapopupcont = RET(metapopupcont);

    vec_t* metafields = VNEW();

    VADDR(metafields, cstr_new_cstring("key"));
    VADDR(metafields, num_new_int(150));
    VADDR(metafields, cstr_new_cstring("value"));
    VADDR(metafields, num_new_int(150));

    ui.metalisttable = ui_table_create(
	"metalisttable",
	metalist,
	NULL,
	metalistevt,
	NULL,
	metafields,
	on_metalist_event);

    REL(metafields);

    view_remove_from_parent(ui.metapopupcont);

    /* get visual views */

    ui.cover       = view_get_subview(ui.view_base, "cover");
    ui.visL        = view_get_subview(ui.view_base, "visL");
    ui.visR        = view_get_subview(ui.view_base, "visR");
    ui.visuals     = RET(view_get_subview(ui.view_base, "visuals"));
    ui.songlisttop = RET(view_get_subview(ui.view_base, "songlisttop"));

    view_gen_texture(ui.visL);
    view_gen_texture(ui.visR);
    view_gen_texture(ui.cover);

    vh_touch_add(ui.visL, btn_cb);
    vh_touch_add(ui.visR, btn_cb);

    REL(btn_cb);

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

    REL(ui.visuals);
    REL(ui.settingspopupcont);
    REL(ui.songlisttop);
    REL(ui.metapopupcont);
    REL(ui.filterpopupcont);

    REL(ui.sizecb);
    REL(ui.view_base);
    REL(ui.metalisttable);
    REL(ui.songlisttable);
    REL(ui.settingslisttable);
    REL(ui.artistlisttable);
    REL(ui.genrelisttable);

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
    if (ui.ms)
    {
	if (ui.hide_visuals == 0)
	{
	    double rem;
	    mp_video_refresh(ui.ms, &rem, ui.cover->texture.bitmap);
	    mp_audio_refresh(ui.ms, ui.visL->texture.bitmap, ui.visR->texture.bitmap);
	    ui.cover->texture.changed = 1;
	    ui.visL->texture.changed  = 1;
	    ui.visR->texture.changed  = 1;
	}

	double time = roundf(mp_get_master_clock(ui.ms) * 10.0) / 10.0;

	if (time != ui.timestate && !isnan(time))
	{
	    ui.timestate = time;

	    int tmin = (int) floor(time / 60.0);
	    int tsec = (int) time % 60;
	    int dmin = (int) floor(ui.ms->duration / 60.0);
	    int dsec = (int) ui.ms->duration % 60;

	    char timebuff[20];
	    snprintf(timebuff, 20, "%.2i:%.2i / %.2i:%.2i", tmin, tsec, dmin, dsec);
	    tg_text_set(ui.timetf, timebuff, ui.timets);

	    double ratio = time / ui.ms->duration;
	    tg_knob_set_angle(ui.seekknob, ratio * 6.28 - 3.14 / 2.0);
	}

	if (ui.ms->finished) ui_play_next();
    }
    else
    {
	gfx_rect(ui.visL->texture.bitmap, 0, ui.visL->texture.bitmap->h / 2, ui.visL->texture.bitmap->w, 1, 0xFFFFFFFF, 1);
	ui.visL->texture.changed = 1;
	gfx_rect(ui.visR->texture.bitmap, 0, ui.visR->texture.bitmap->h / 2, ui.visR->texture.bitmap->w, 1, 0xFFFFFFFF, 1);
	ui.cover->texture.changed = 1;
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
    lib_get_entries(entries);
    songlist_set_songs(entries);
    ui_set_songs(songlist_get_visible_songs());
    REL(entries);
}

#endif
