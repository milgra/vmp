#ifndef ui_h
#define ui_h

#include "view.c"

void ui_init(float width, float height);
void ui_destroy();

void ui_play_next();
void ui_play_prev();
void ui_toggle_pause();

void ui_add_cursor();
void ui_update_cursor(r2_t frame);
void ui_screenshot(uint32_t time);

void ui_update_player();
void ui_update_songlist();
void ui_show_progress(char* progress);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "bm_rgba_util.c"
#include "coder.c"
#include "config.c"
#include "cstr_util.c"
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
#include "views.c"
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

    ui_table_t* metatable;
    ui_table_t* songtable;
    ui_table_t* genretable;
    ui_table_t* artisttable;
    ui_table_t* settingstable;

    view_t* metapopupcont;
    view_t* filterpopupcont;
    view_t* settingspopupcont;

    cb_t* size_cb;

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

    view_t* metashadow;
    view_t* metaacceptbtn;

    textstyle_t infots;
    textstyle_t timets;
    textstyle_t inputts;
    textstyle_t filterts;

    view_t* inputarea;
    view_t* inputbck;
    view_t* inputtf;

    map_t* edited_song;
    char*  edited_key;

    map_t* played_song;
    int    shuffle;
    float  volume;
    int    visutype; // 0 - waves 1 - rdft
    int    hide_visuals;
    float  timestate;
} ui;

int ui_comp_value(void* left, void* right)
{
    map_t* l = left;
    map_t* r = right;

    char* la = MGET(l, "key");
    char* ra = MGET(r, "key");

    return strcmp(la, ra);
}

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

    ui.ms = mp_open(realpath, ui.size_cb);
    mp_set_volume(ui.ms, ui.volume);
    mp_set_visutype(ui.ms, ui.visutype);

    /* clear cover view */
    gfx_rect(ui.cover->texture.bitmap, 0, 0, ui.cover->texture.bitmap->w, ui.cover->texture.bitmap->h, 0x151515FF, 1);

    /* update song list table */
    uint32_t index = songlist_get_index(song);
    if (index < UINT32_MAX) ui_table_select(ui.songtable, index);

    /* update info text */
    char info[200];
    snprintf(info, 200, "%s / %s", (char*) MGET(song, "artist"), (char*) MGET(song, "title"));
    tg_text_set(ui.infotf, info, ui.infots);

    view_t* playbtn = view_get_subview(ui.view_base, "playbtn");
    if (playbtn) vh_button_set_state(playbtn, VH_BUTTON_DOWN);
}

void ui_cancel_input()
{
    view_remove_subview(ui.view_base, ui.inputarea);
}

void ui_on_cancel_inputtf(view_t* view)
{
    ui_cancel_input();
}

void ui_on_return_inputtf(view_t* view)
{
    // save value of inputtf

    view_add_subview(ui.metashadow, ui.metaacceptbtn);
    ui_cancel_input();

    str_t* val  = vh_textinput_get_text(view);
    char*  sval = str_new_cstring(val);

    MPUT(ui.edited_song, ui.edited_key, sval);
    mem_describe(ui.edited_song, 0);

    vec_t* pairs = VNEW();
    vec_t* keys  = VNEW();
    map_keys(ui.edited_song, keys);

    for (int index = 0; index < keys->length; index++)
    {
	char*  key   = keys->data[index];
	char*  value = MGET(ui.edited_song, key);
	map_t* map   = MNEW();
	MPUT(map, "key", key);
	MPUT(map, "value", value);
	VADDR(pairs, map);
    }

    vec_sort(pairs, ui_comp_value);

    ui_table_set_data(ui.metatable, pairs);

    // ui.edited_song
    // ui.edited_key

    REL(sval);
}

void ui_on_key_down(void* userdata, void* data)
{
    ev_t* ev = (ev_t*) data;
    if (ev->keycode == SDLK_SPACE) ui_toggle_pause();
}

void ui_on_touch(void* userdata, void* data)
{
    view_t* view = data;

    if (strcmp(view->id, "inputarea") == 0)
    {
	printf("ONTOUCH\n");
	ui_cancel_input();
    }
}

void ui_content_size(void* userdata, void* data)
{
    /* handle aspect ratio of cover / video */
    /* v2_t* r = (v2_t*) data; */
}

void ui_on_btn_event(void* userdata, void* data)
{
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
    if (strcmp(btnview->id, "metabtn") == 0)
    {
	if (!ui.metapopupcont->parent)
	{
	    view_add_subview(ui.view_base, ui.metapopupcont);

	    if (ui.songtable->selected_items->length > 0)
	    {
		map_t* info = ui.songtable->selected_items->data[0];

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

		vec_sort(pairs, ui_comp_value);

		ui_table_set_data(ui.metatable, pairs);
		REL(pairs);
		REL(keys);

		view_layout(ui.view_base);

		ui.edited_song = info;
	    }
	}
    };
    if (strcmp(btnview->id, "filterbtn") == 0)
    {
	if (!ui.filterpopupcont->parent)
	{
	    view_add_subview(ui.view_base, ui.filterpopupcont);

	    vec_t* genres  = VNEW();
	    vec_t* artists = VNEW();

	    lib_get_genres(genres);
	    lib_get_artists(artists);

	    ui_table_set_data(ui.genretable, genres);
	    ui_table_set_data(ui.artisttable, artists);

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
    if (strcmp(btnview->id, "inputclearbtn") == 0)
    {
	vh_textinput_set_text(ui.inputtf, "");
	ui_manager_activate(ui.inputtf);
	vh_textinput_activate(ui.inputtf, 1);
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

void on_songlist_event(ui_table_event event)
{
    switch (event.id)
    {
	case UI_TABLE_EVENT_FIELDS_UPDATE:
	{
	    vec_t* fields   = event.fields;
	    char*  fieldstr = cstr_new_cstring("");
	    for (int index = 0; index < fields->length; index += 2)
	    {
		char*  field = fields->data[index];
		num_t* value = fields->data[index + 1];
		char*  pair  = cstr_new_format(100, "%s %i ", field, value->intv);
		fieldstr     = cstr_append(fieldstr, pair);
		REL(pair);
	    }
	    config_set("fields", fieldstr);
	    config_write(config_get("cfg_path"));
	}
	break;
	case UI_TABLE_EVENT_FIELD_SELECT:
	{
	    char* field   = event.field;
	    char* sorting = cstr_new_cstring(songlist_get_sorting());

	    if (strstr(sorting, field) != NULL)
	    {
		char* part  = strstr(sorting, field);
		char  value = part[strlen(field) + 1];

		if (strcmp(field, "artist") == 0)
		    sorting = cstr_new_format(100, "%s %c album 1 track 1", field, value == '0' ? '1' : '0');
		else
		    sorting = cstr_new_format(100, "%s %c", field, value == '0' ? '1' : '0');
	    }
	    else
	    {
		if (strcmp(field, "artist") == 0)
		    sorting = cstr_new_format(100, "%s 1 album 1 track 1", field);
		else
		    sorting = cstr_new_format(100, "%s 1", field);
	    }

	    songlist_set_sorting(sorting);

	    REL(sorting);

	    config_set("sorting", sorting);
	    config_write(config_get("cfg_path"));

	    ui_update_songlist();
	}
	break;
	case UI_TABLE_EVENT_SELECT:
	{
	}
	break;
	case UI_TABLE_EVENT_OPEN:
	{
	    vec_t* selected = event.selected_items;
	    map_t* info     = selected->data[0];

	    ui_play_song(info);
	}
	break;
	case UI_TABLE_EVENT_DRAG:
	{
	}
	break;
	case UI_TABLE_EVENT_KEY:
	{
	    ev_t ev = event.event;

	    if (ev.keycode == SDLK_DOWN || ev.keycode == SDLK_UP)
	    {
		int32_t index = event.table->selected_index;

		if (ev.keycode == SDLK_DOWN) index += 1;
		if (ev.keycode == SDLK_UP) index -= 1;
		ui_table_select(event.table, index);
	    }
	}
	break;
	case UI_TABLE_EVENT_DROP:
	    break;
    }
}

void on_settingslist_event(ui_table_event event)
{
    switch (event.id)
    {
	case UI_TABLE_EVENT_SELECT:
	{
	}
	break;
    }
}

void on_genrelist_event(ui_table_event event)
{
    switch (event.id)
    {
	case UI_TABLE_EVENT_SELECT:
	{
	    vec_t* selected = event.selected_items;
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

void on_artistlist_event(ui_table_event event)
{
    switch (event.id)
    {
	case UI_TABLE_EVENT_SELECT:
	{
	    vec_t* selected = event.selected_items;
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

void on_metalist_event(ui_table_event event)
{
    switch (event.id)
    {
	case UI_TABLE_EVENT_SELECT:
	{
	    break;
	}
	case UI_TABLE_EVENT_OPEN:
	{
	    // show input textfield above rowwiew

	    if (event.rowview)
	    {
		// show value in input textfield
		map_t* info = event.selected_items->data[0];
		char*  key  = MGET(info, "key");

		if (strcmp(key, "artist") == 0 ||
		    strcmp(key, "album") == 0 ||
		    strcmp(key, "title") == 0 ||
		    strcmp(key, "genre") == 0)
		{
		    view_t* valueview = event.rowview->views->data[1];
		    r2_t    rframe    = valueview->frame.global;
		    r2_t    iframe    = ui.inputbck->frame.global;
		    iframe.x          = rframe.x;
		    iframe.y          = rframe.y - 5;
		    view_set_frame(ui.inputbck, iframe);

		    view_add_subview(ui.view_base, ui.inputarea);

		    view_layout(ui.view_base);

		    ui_manager_activate(ui.inputtf);
		    vh_textinput_activate(ui.inputtf, 1);

		    char* value = MGET(info, "value");
		    if (!value) value = "";

		    if (value)
		    {
			vh_textinput_set_text(ui.inputtf, value);

			ui.edited_key = MGET(info, "key");
		    }
		}
	    }
	    break;
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

    REL(filter);
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

void ui_update_songlist()
{
    vec_t* entries = VNEW();
    lib_get_entries(entries);
    songlist_set_songs(entries);
    ui_table_set_data(ui.songtable, songlist_get_visible_songs());
    REL(entries);
}

void ui_init(float width, float height)
{
    text_init();                    // DESTROY 0
    ui_manager_init(width, height); // DESTROY 1

    /* generate views from descriptors */

    vec_t* view_list = VNEW();

    viewgen_html_parse(config_get("html_path"), view_list);
    viewgen_css_apply(view_list, config_get("css_path"), config_get("res_path"));
    viewgen_type_apply(view_list);

    ui.view_base = RET(vec_head(view_list));

    REL(view_list);

    /* initial layout of views */

    view_set_frame(ui.view_base, (r2_t){0.0, 0.0, (float) width, (float) height});
    ui_manager_add(ui.view_base);

    /* callbacks */

    cb_t* btn_cb   = cb_new(ui_on_btn_event, NULL);
    cb_t* key_cb   = cb_new(ui_on_key_down, NULL);
    cb_t* size_cb  = cb_new(ui_content_size, NULL);
    cb_t* touch_cb = cb_new(ui_on_touch, NULL);

    ui.size_cb = size_cb;

    /* listen for keys and shortcuts */

    vh_key_add(ui.view_base, key_cb);
    ui.view_base->needs_key = 1;

    /* knobs */

    view_t* seekknob = view_get_subview(ui.view_base, "seekknob");
    view_t* volknob  = view_get_subview(ui.view_base, "volknob");

    tg_knob_add(seekknob);
    vh_knob_add(seekknob, ui_pos_change);

    tg_knob_add(volknob);
    vh_knob_add(volknob, ui_vol_change);

    ui.volknob  = volknob;
    ui.seekknob = seekknob;

    /* setup volume */

    ui.volume = 0.8;
    tg_knob_set_angle(ui.volknob, ui.volume * 6.28 - 3.14 / 2.0);

    /* buttons */

    view_t* prevbtn          = view_get_subview(ui.view_base, "prevbtn");
    view_t* nextbtn          = view_get_subview(ui.view_base, "nextbtn");
    view_t* shufflebtn       = view_get_subview(ui.view_base, "shufflebtn");
    view_t* metabtn          = view_get_subview(ui.view_base, "metabtn");
    view_t* settingsbtn      = view_get_subview(ui.view_base, "settingsbtn");
    view_t* visubtn          = view_get_subview(ui.view_base, "visubtn");
    view_t* maxbtn           = view_get_subview(ui.view_base, "maxbtn");
    view_t* exitbtn          = view_get_subview(ui.view_base, "exitbtn");
    view_t* filterbtn        = view_get_subview(ui.view_base, "filterbtn");
    view_t* clearbtn         = view_get_subview(ui.view_base, "clearbtn");
    view_t* inputclearbtn    = view_get_subview(ui.view_base, "inputclearbtn");
    view_t* filterclosebtn   = view_get_subview(ui.view_base, "filterclosebtn");
    view_t* settingsclosebtn = view_get_subview(ui.view_base, "settingsclosebtn");
    view_t* playbtn          = view_get_subview(ui.view_base, "playbtn");
    view_t* mutebtn          = view_get_subview(ui.view_base, "mutebtn");

    vh_button_add(prevbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(nextbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(shufflebtn, VH_BUTTON_TOGGLE, btn_cb);
    vh_button_add(metabtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(settingsbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(visubtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(maxbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(exitbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(filterbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(clearbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(inputclearbtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(playbtn, VH_BUTTON_TOGGLE, btn_cb);
    vh_button_add(mutebtn, VH_BUTTON_TOGGLE, btn_cb);
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

    ui.inputarea = RET(view_get_subview(ui.view_base, "inputarea"));
    ui.inputbck  = view_get_subview(ui.view_base, "inputbck");
    ui.inputtf   = view_get_subview(ui.view_base, "inputtf");
    ui.inputts   = ui_util_gen_textstyle(ui.inputtf);

    ui.inputbck->blocks_touch   = 1;
    ui.inputarea->blocks_touch  = 1;
    ui.inputarea->blocks_scroll = 1;

    vh_textinput_add(ui.inputtf, "Generic input", "", ui.inputts, NULL);
    vh_textinput_set_on_deactivate(ui.inputtf, ui_on_cancel_inputtf);
    vh_textinput_set_on_return(ui.inputtf, ui_on_return_inputtf);
    vh_textinput_set_deactivate_on_mouse_out(ui.inputtf, 0);

    vh_touch_add(ui.inputarea, touch_cb);

    view_remove_from_parent(ui.inputarea);

    /* songlist */

    char*  fieldconfig = config_get("fields");
    vec_t* words       = cstr_split(fieldconfig, " ");
    vec_t* fields      = VNEW();

    for (int index = 0; index < words->length; index += 2)
    {
	char* field = words->data[index];
	char* value = words->data[index + 1];

	VADD(fields, field);
	VADDR(fields, num_new_int(atoi(value)));
    }

    REL(words);

    view_t* songlist       = view_get_subview(ui.view_base, "songtable");
    view_t* songlistscroll = view_get_subview(ui.view_base, "songlistscroll");
    view_t* songlistevt    = view_get_subview(ui.view_base, "songlistevt");
    view_t* songlisthead   = view_get_subview(ui.view_base, "songlisthead");

    ui.songtable = ui_table_create(
	"songtable",
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
    view_t* settingslist      = view_get_subview(ui.view_base, "settingstable");
    view_t* settingslistevt   = view_get_subview(ui.view_base, "settingslistevt");

    settingspopup->blocks_touch  = 1;
    settingspopup->blocks_scroll = 1;

    ui.settingspopupcont = RET(settingspopupcont);

    fields = VNEW();

    VADDR(fields, cstr_new_cstring("value"));
    VADDR(fields, num_new_int(510));

    ui.settingstable = ui_table_create(
	"settingstable",
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

    ui_table_set_data(ui.settingstable, items);
    REL(items);

    view_remove_from_parent(ui.settingspopupcont);

    /* genre and artist lists */

    view_t* filterpopupcont = view_get_subview(ui.view_base, "filterpopupcont");
    view_t* filterpopup     = view_get_subview(ui.view_base, "filterpopup");
    view_t* genrelist       = view_get_subview(ui.view_base, "genretable");
    view_t* genrelistevt    = view_get_subview(ui.view_base, "genrelistevt");
    view_t* artistlist      = view_get_subview(ui.view_base, "artisttable");
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

    ui.genretable = ui_table_create(
	"genretable",
	genrelist,
	NULL,
	genrelistevt,
	NULL,
	genrefields,
	on_genrelist_event);

    ui.artisttable = ui_table_create(
	"artisttable",
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
    view_t* metalist      = view_get_subview(ui.view_base, "metatable");
    view_t* metalistevt   = view_get_subview(ui.view_base, "metalistevt");

    view_t* metaclosebtn  = view_get_subview(ui.view_base, "metaclosebtn");
    view_t* metaacceptbtn = view_get_subview(ui.view_base, "metaacceptbtn");

    vh_button_add(metaclosebtn, VH_BUTTON_NORMAL, btn_cb);
    vh_button_add(metaacceptbtn, VH_BUTTON_NORMAL, btn_cb);

    ui.metashadow    = view_get_subview(ui.view_base, "metashadow");
    ui.metaacceptbtn = RET(metaacceptbtn);

    view_remove_from_parent(ui.metaacceptbtn);

    metapopup->blocks_touch  = 1;
    metapopup->blocks_scroll = 1;

    ui.metapopupcont = RET(metapopupcont);

    vec_t* metafields = VNEW();

    VADDR(metafields, cstr_new_cstring("key"));
    VADDR(metafields, num_new_int(100));
    VADDR(metafields, cstr_new_cstring("value"));
    VADDR(metafields, num_new_int(350));

    ui.metatable = ui_table_create(
	"metatable",
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
    REL(key_cb);

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

    REL(ui.size_cb);
    REL(ui.view_base);

    REL(ui.metatable);
    REL(ui.songtable);
    REL(ui.settingstable);
    REL(ui.artisttable);
    REL(ui.genretable);

    REL(ui.metaacceptbtn);
    REL(ui.inputarea);

    ui_manager_destroy(); // DESTROY 1

    text_destroy(); // DESTROY 0
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

void ui_screenshot(uint32_t time)
{
    if (config_get("lib_path"))
    {
	static int cnt    = 0;
	view_t*    root   = ui_manager_get_root();
	r2_t       frame  = root->frame.local;
	bm_rgba_t* screen = bm_rgba_new(frame.w, frame.h); // REL 0

	// remove cursor for screenshot to remain identical

	if (ui.cursor)
	{
	    ui_manager_remove(ui.cursor);
	    ui_manager_render(time);
	    ui_manager_add_to_top(ui.cursor);
	}

	ui_compositor_render_to_bmp(screen);

	char*      name    = cstr_new_format(20, "screenshot%.3i.png", cnt++); // REL 1
	char*      path    = path_new_append(config_get("lib_path"), name);    // REL 2
	bm_rgba_t* flipped = bm_rgba_new_flip_y(screen);                       // REL 3

	coder_write_png(path, flipped);

	REL(flipped); // REL 3
	REL(name);    // REL 2
	REL(path);    // REL 1
	REL(screen);  // REL 0

	if (ui.cursor) ui_update_cursor(frame); // full screen cursor to indicate screenshot, next step will reset it
    }
}

void ui_update_player()
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

void ui_show_progress(char* progress)
{
    tg_text_set(ui.infotf, progress, ui.infots);
}

#endif
