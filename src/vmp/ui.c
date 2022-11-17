#ifndef ui_h
#define ui_h

#include "ku_view.c"
#include "ku_window.c"

void ui_init(int width, int height, float scale, ku_window_t* window);
void ui_destroy();

void ui_play_next();
void ui_play_prev();
void ui_toggle_pause();

void ui_update_player();
void ui_update_songlist();
void ui_show_progress(char* progress);

void ui_add_cursor();
void ui_update_cursor(ku_rect_t frame);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "coder.c"
#include "config.c"
#include "filemanager.c"
#include "ku_bitmap.c"
#include "ku_connector_wayland.c"
#include "ku_draw.c"
#include "ku_gen_css.c"
#include "ku_gen_html.c"
#include "ku_gen_textstyle.c"
#include "ku_gen_type.c"
#include "ku_table.c"
#include "ku_text.c"
#include "library.c"
#include "mediaplayer.c"
#include "mt_bitmap_ext.c"
#include "mt_log.c"
#include "mt_map_ext.c"
#include "mt_number.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_string_ext.c"
#include "mt_time.c"
#include "songlist.c"
#include "tg_css.c"
#include "tg_knob.c"
#include "tg_text.c"
#include "vh_button.c"
#include "vh_key.c"
#include "vh_knob.c"
#include "vh_textinput.c"
#include "vh_touch.c"
#include <xkbcommon/xkbcommon.h>

typedef enum _ui_inputmode ui_inputmode;
enum _ui_inputmode
{
    UI_IM_SORTING,
    UI_IM_EDITING,
    UI_IM_COVERART
};

struct _ui_t
{
    ku_window_t* window; /* window for this ui */

    ku_table_t* metatable;
    ku_table_t* songtable;
    ku_table_t* genretable;
    ku_table_t* artisttable;
    ku_table_t* settingstable;
    ku_table_t* contexttable;

    ku_view_t* metapopupcont;
    ku_view_t* filterpopupcont;
    ku_view_t* settingspopupcont;
    ku_view_t* contextpopupcont;

    ku_view_t* cursor;
    ku_view_t* view_base;

    ku_view_t* visL;
    ku_view_t* visR;
    ku_view_t* cover;
    ku_view_t* visuals;
    ku_view_t* songlisttop;

    ku_view_t* volknob;
    ku_view_t* seekknob;

    ku_view_t* infotf;
    ku_view_t* timetf;
    ku_view_t* filtertf;

    ku_view_t* metashadow;
    ku_view_t* metaacceptbtn;

    ku_view_t* inputarea;
    ku_view_t* inputbck;
    ku_view_t* inputtf;

    MediaState_t* ms;

    mt_map_t*    edited_song;
    char*        edited_key;
    char*        edited_cover;
    mt_map_t*    edited_changed;
    mt_vector_t* edited_deleted;

    mt_map_t* played_song;
    int       shuffle;
    float     volume;
    int       visutype; // 0 - waves 1 - rdft
    int       hide_visuals;
    float     timestate;

    ui_inputmode inputmode;
    ku_view_t*   cursorv; /* replay cursor */

} ui;

int ui_comp_value(void* left, void* right)
{
    mt_map_t* l = left;
    mt_map_t* r = right;

    char* la = MGET(l, "key");
    char* ra = MGET(r, "key");

    return strcmp(la, ra);
}

void ui_on_media_player_event(ms_event_t event)
{
    /* handle aspect ratio of cover / video */
    /* v2_t* r = (v2_t*) data; */
}

void ui_play_song(mt_map_t* song)
{
    /* increase play/skip counter */
    if (ui.played_song)
    {
	if (ui.ms->finished)
	{
	    int plays = atoi(MGET(ui.played_song, "plays"));
	    MPUTR(ui.played_song, "plays", STRNF(10, "%i", ++plays));
	}
	else
	{
	    int skips = atoi(MGET(ui.played_song, "skips"));
	    MPUTR(ui.played_song, "skips", STRNF(10, "%i", ++skips));
	}
	ui_update_songlist();
	lib_write(config_get("lib_path"));
    }

    /* close existing ms */
    if (ui.ms) mp_close(ui.ms);
    ui.ms = NULL;

    /* update played song */
    if (ui.played_song) REL(ui.played_song);
    ui.played_song = RET(song);

    /* create new ms */
    char* path     = MGET(song, "path");
    char* realpath = mt_path_new_append(config_get("lib_path"), path);

    ui.ms = mp_open(realpath, ui_on_media_player_event);

    REL(realpath);

    mp_set_volume(ui.ms, ui.volume);
    mp_set_visutype(ui.ms, ui.visutype);

    /* clear cover view */
    ku_draw_rect(ui.cover->texture.bitmap, 0, 0, ui.cover->texture.bitmap->w, ui.cover->texture.bitmap->h, 0x151515FF, 1);

    /* update song list table */
    uint32_t index = songlist_get_index(song);
    if (index < UINT32_MAX) ku_table_select(ui.songtable, index, 0);

    /* update info text */
    char info[200];
    snprintf(info, 200, "%s / %s", (char*) MGET(song, "artist"), (char*) MGET(song, "title"));
    tg_text_set1(ui.infotf, info);

    ku_view_t* playbtn = ku_view_get_subview(ui.view_base, "playbtn");
    if (playbtn) vh_button_set_state(playbtn, VH_BUTTON_DOWN);
}

void ui_open_metadata_editor()
{
    if (!ui.metapopupcont->parent)
    {
	mt_map_reset(ui.edited_changed);
	mt_vector_reset(ui.edited_deleted);

	ku_view_add_subview(ui.view_base, ui.metapopupcont);

	if (ui.songtable->selected_items->length > 0)
	{
	    mt_map_t* info = ui.songtable->selected_items->data[0];

	    ku_view_t* cover    = ku_view_get_subview(ui.metapopupcont, "metacover");
	    char*      path     = MGET(info, "path");
	    char*      realpath = mt_path_new_append(config_get("lib_path"), path);

	    if (!cover->texture.bitmap) ku_view_gen_texture(cover);

	    int success = coder_load_cover_into(realpath, cover->texture.bitmap);

	    if (!success) ku_view_gen_texture(cover);

	    cover->texture.changed = 1;

	    REL(realpath);

	    mt_vector_t* pairs = VNEW();
	    mt_vector_t* keys  = VNEW();
	    mt_map_keys(info, keys);
	    for (int index = 0; index < keys->length; index++)
	    {
		char*     key   = keys->data[index];
		char*     value = MGET(info, key);
		mt_map_t* map   = MNEW();
		MPUT(map, "key", key);
		MPUT(map, "value", value);
		VADDR(pairs, map);
	    }

	    mt_vector_sort(pairs, ui_comp_value);

	    ku_table_set_data(ui.metatable, pairs);
	    REL(pairs);
	    REL(keys);

	    ku_view_layout(ui.view_base);

	    ui.edited_song = info;
	}
    }
}

void ui_cancel_input()
{
    ku_view_remove_subview(ui.view_base, ui.inputarea);
}

void ui_on_key_down(vh_key_event_t event)
{
    if (event.ev.keycode == XKB_KEY_space) ui_toggle_pause();
}

void ui_on_touch(vh_touch_event_t event)
{
    if (strcmp(event.view->id, "inputarea") == 0)
    {
	ui_cancel_input();
    }
    if (strcmp(event.view->id, "contextpopupcont") == 0)
    {
	ku_view_remove_from_parent(ui.contextpopupcont);
    }
    if (strcmp(event.view->id, "visL") == 0)
    {
	ui.visutype = 1 - ui.visutype;
	if (ui.ms) mp_set_visutype(ui.ms, ui.visutype);
    }
    if (strcmp(event.view->id, "visR") == 0)
    {
	ui.visutype = 1 - ui.visutype;
	if (ui.ms) mp_set_visutype(ui.ms, ui.visutype);
    }
}

void ui_on_btn_event(vh_button_event_t event)
{
    if (strcmp(event.view->id, "playbtn") == 0)
    {
	if (event.vh->state == VH_BUTTON_DOWN)
	{
	    if (ui.ms) mp_play(ui.ms);
	    else
	    {
		mt_map_t* song = songlist_get_song(0);
		if (song) ui_play_song(song);
	    }
	}
	else
	{
	    if (ui.ms) mp_pause(ui.ms);
	}
    };
    if (strcmp(event.view->id, "mutebtn") == 0)
    {
	if (ui.ms)
	{
	    if (event.vh->state == VH_BUTTON_DOWN) mp_mute(ui.ms);
	    else mp_unmute(ui.ms);
	}
    };
    if (strcmp(event.view->id, "prevbtn") == 0)
    {
	mt_map_t* prev = NULL;

	if (ui.shuffle == 0 && ui.played_song) prev = songlist_get_prev_song(ui.played_song);
	else prev = songlist_get_song(1);

	if (prev) ui_play_song(prev);
    };
    if (strcmp(event.view->id, "nextbtn") == 0)
    {
	mt_map_t* next = NULL;

	if (ui.shuffle == 0 && ui.played_song) next = songlist_get_next_song(ui.played_song);
	else next = songlist_get_song(1);

	if (next) ui_play_song(next);
    };
    if (strcmp(event.view->id, "shufflebtn") == 0)
    {
	ui.shuffle = 1 - ui.shuffle;
    };
    if (strcmp(event.view->id, "settingsbtn") == 0)
    {
	if (!ui.settingspopupcont->parent)
	{
	    ku_view_add_subview(ui.view_base, ui.settingspopupcont);
	    ku_view_layout(ui.view_base);
	}
    };
    if (strcmp(event.view->id, "visubtn") == 0)
    {
	ui.hide_visuals = 1 - ui.hide_visuals;

	ku_view_t* songs = ku_view_get_subview(ui.view_base, "songs");

	if (ui.hide_visuals)
	{
	    ku_view_remove_from_parent(ui.visuals);
	    ku_view_remove_from_parent(ui.songlisttop);
	}
	else
	{
	    ku_view_insert_subview(songs, ui.songlisttop, 0);
	    ku_view_add_subview(ui.view_base, ui.visuals);
	}
	ku_view_layout(ui.view_base);
    };
    if (strcmp(event.view->id, "metabtn") == 0)
    {
	ui_open_metadata_editor();
    };
    if (strcmp(event.view->id, "filterbtn") == 0)
    {
	if (!ui.filterpopupcont->parent)
	{
	    ku_view_add_subview(ui.view_base, ui.filterpopupcont);

	    mt_vector_t* genres  = VNEW();
	    mt_vector_t* artists = VNEW();

	    lib_get_genres(genres);
	    lib_get_artists(artists);

	    printf("artisttable %i\n", ui.artisttable == NULL);

	    ku_table_set_data(ui.genretable, genres);
	    ku_table_set_data(ui.artisttable, artists);

	    REL(genres);
	    REL(artists);

	    ku_view_layout(ui.view_base);

	    ku_window_activate(ui.window, ui.filtertf);
	    vh_textinput_activate(ui.filtertf, 1);
	}
    };
    if (strcmp(event.view->id, "sortingbtn") == 0)
    {
	// show inputfield with sorting options
	ku_rect_t rframe = ui.view_base->frame.global;
	ku_rect_t iframe = ui.inputbck->frame.global;
	iframe.x         = rframe.w / 2 - iframe.w / 2;
	iframe.y         = rframe.h / 2 - iframe.h / 2;

	ku_view_set_frame(ui.inputbck, iframe);
	ku_view_add_subview(ui.view_base, ui.inputarea);
	ku_view_layout(ui.view_base);

	ku_window_activate(ui.window, ui.inputtf);
	vh_textinput_activate(ui.inputtf, 1);

	vh_textinput_set_text(ui.inputtf, config_get("sorting"));

	ui.inputmode = UI_IM_SORTING;
    };
    if (strcmp(event.view->id, "clearbtn") == 0)
    {
	vh_textinput_set_text(ui.filtertf, "");
	ku_window_activate(ui.window, ui.filtertf);
	vh_textinput_activate(ui.filtertf, 1);

	songlist_set_filter(NULL);
	ui_update_songlist();
    };
    if (strcmp(event.view->id, "inputclearbtn") == 0)
    {
	vh_textinput_set_text(ui.inputtf, "");
	ku_window_activate(ui.window, ui.inputtf);
	vh_textinput_activate(ui.inputtf, 1);
    };
    if (strcmp(event.view->id, "exitbtn") == 0) ku_wayland_exit();
    if (strcmp(event.view->id, "maxbtn") == 0) ku_wayland_toggle_fullscreen();
    if (strcmp(event.view->id, "settingsclosebtn") == 0)
    {
	ku_view_remove_from_parent(ui.settingspopupcont);
    }
    if (strcmp(event.view->id, "filterclosebtn") == 0)
    {
	ku_view_remove_from_parent(ui.filterpopupcont);
    }
    if (strcmp(event.view->id, "metaclosebtn") == 0)
    {
	ku_view_remove_from_parent(ui.metapopupcont);
    }
    if (strcmp(event.view->id, "metaacceptbtn") == 0)
    {
	printf("CAHNEGED\n");

	mt_memory_describe(ui.edited_changed, 0);

	char* path   = MGET(ui.edited_song, "path");
	int   result = coder_write_metadata(config_get("lib_path"), path, ui.edited_cover, ui.edited_changed, ui.edited_deleted);
	if (result >= 0)
	{
	    printf("metadata upadted\n");
	    // modify song in db also if metadata is successfully written into file
	    // db_update_metadata(path, ep.changed, ep.removed);
	}

	ku_view_remove_from_parent(ui.metapopupcont);
	lib_write(config_get("lib_path"));
	ui_update_songlist();
    }
    if (strcmp(event.view->id, "metacoverbtn") == 0)
    {
	// show inputfield with sorting options
	ku_rect_t rframe = ui.view_base->frame.global;
	ku_rect_t iframe = ui.inputbck->frame.global;
	iframe.x         = rframe.w / 2 - iframe.w / 2;
	iframe.y         = rframe.h / 2 - iframe.h / 2;

	ku_view_set_frame(ui.inputbck, iframe);
	ku_view_add_subview(ui.view_base, ui.inputarea);
	ku_view_layout(ui.view_base);

	ku_window_activate(ui.window, ui.inputtf);
	vh_textinput_activate(ui.inputtf, 1);

	vh_textinput_set_text(ui.inputtf, "/home/path_to_image/image.png");

	ui.inputmode = UI_IM_COVERART;
    }
}

void ui_on_text_event(vh_textinput_event_t event)
{
    if (strcmp(event.view->id, "filtertf") == 0)
    {
	if (event.id == VH_TEXTINPUT_DEACTIVATE)
	{
	    vh_textinput_activate(event.view, 0);
	    ku_window_deactivate(ui.window, event.view);
	}

	songlist_set_filter(event.text);
	ui_update_songlist();
    }
    else if (strcmp(event.view->id, "inputtf") == 0)
    {
	if (event.id == VH_TEXTINPUT_DEACTIVATE) ui_cancel_input();

	if (event.id == VH_TEXTINPUT_RETURN)
	{
	    if (ui.inputmode == UI_IM_EDITING)
	    {
		// save value of inputtf

		ku_view_add_subview(ui.metashadow, ui.metaacceptbtn);
		ui_cancel_input();

		MPUT(ui.edited_song, ui.edited_key, event.text);
		MPUT(ui.edited_changed, ui.edited_key, event.text);

		mt_vector_t* pairs = VNEW();
		mt_vector_t* keys  = VNEW();
		mt_map_keys(ui.edited_song, keys);

		for (int index = 0; index < keys->length; index++)
		{
		    char*     key   = keys->data[index];
		    char*     value = MGET(ui.edited_song, key);
		    mt_map_t* map   = MNEW();
		    MPUT(map, "key", key);
		    MPUT(map, "value", value);
		    VADDR(pairs, map);
		}

		mt_vector_sort(pairs, ui_comp_value);

		ku_table_set_data(ui.metatable, pairs);
	    }
	    else if (ui.inputmode == UI_IM_SORTING)
	    {
		mt_vector_t* words = mt_string_split(event.text, " ");

		if (words->length % 2 == 0)
		{
		    songlist_set_sorting(event.text);

		    config_set("sorting", event.text);
		    config_write(config_get("cfg_path"));

		    ui_update_songlist();
		}

		REL(words);
	    }
	    else if (ui.inputmode == UI_IM_COVERART)
	    {
		char* path_final = mt_path_new_normalize(event.text, config_get("wrk_path")); // REL 1
		// check if image is valid
		ku_bitmap_t* image = coder_load_image(path_final);

		printf("PATH %s\n", path_final);

		if (image)
		{
		    printf("loading image\n");
		    ku_view_t* cover = ku_view_get_subview(ui.metapopupcont, "metacover");

		    if (!cover->texture.bitmap) ku_view_gen_texture(cover);

		    coder_load_image_into(path_final, cover->texture.bitmap);

		    cover->texture.changed = 1;

		    ku_view_add_subview(ui.metashadow, ui.metaacceptbtn);
		    ui_cancel_input();

		    ui.edited_cover = RET(path_final);
		}

		REL(path_final); // REL 1
	    }
	}
    }
}

void ui_pos_change(ku_view_t* view, float angle)
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

    mp_set_position(ui.ms, ratio);
}

void ui_vol_change(ku_view_t* view, float angle)
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
    mt_vector_t* entries = VNEW();
    lib_get_entries(entries);
    songlist_set_songs(entries);
    ku_table_set_data(ui.songtable, songlist_get_visible_songs());
    REL(entries);
}

void on_table_event(ku_table_event_t event)
{
    if (event.table == ui.songtable)
    {
	if (event.id == KU_TABLE_EVENT_FIELDS_UPDATE)
	{
	    mt_vector_t* fields   = event.fields;
	    char*        fieldstr = mt_string_new_cstring("");
	    for (int index = 0; index < fields->length; index += 2)
	    {
		char*        field = fields->data[index];
		mt_number_t* value = fields->data[index + 1];
		char*        pair  = STRNF(100, "%s %i ", field, value->intv);
		fieldstr           = mt_string_append(fieldstr, pair);
		REL(pair);
	    }
	    config_set("fields", fieldstr);
	    config_write(config_get("cfg_path"));
	}
	else if (event.id == KU_TABLE_EVENT_FIELD_SELECT)
	{
	    char* field   = event.field;
	    char* sorting = mt_string_new_cstring(songlist_get_sorting());

	    if (strstr(sorting, field) != NULL)
	    {
		char* part  = strstr(sorting, field);
		char  value = part[strlen(field) + 1];

		if (strcmp(field, "artist") == 0)
		    sorting = STRNF(100, "%s %c album 1 track 1", field, value == '0' ? '1' : '0');
		else
		    sorting = STRNF(100, "%s %c", field, value == '0' ? '1' : '0');
	    }
	    else
	    {
		if (strcmp(field, "artist") == 0)
		    sorting = STRNF(100, "%s 1 album 1 track 1", field);
		else
		    sorting = STRNF(100, "%s 1", field);
	    }

	    songlist_set_sorting(sorting);

	    REL(sorting);

	    config_set("sorting", sorting);
	    config_write(config_get("cfg_path"));

	    ui_update_songlist();
	}
	else if (event.id == KU_TABLE_EVENT_SELECT)
	{
	}
	else if (event.id == KU_TABLE_EVENT_CONTEXT)
	{
	    if (ui.contextpopupcont->parent == NULL)
	    {
		ku_view_add_subview(ui.view_base, ui.contextpopupcont);
		ku_view_layout(ui.view_base);
	    }
	}
	else if (event.id == KU_TABLE_EVENT_OPEN)
	{
	    mt_vector_t* selected = event.selected_items;
	    mt_map_t*    info     = selected->data[0];

	    ui_play_song(info);
	}
	else if (event.id == KU_TABLE_EVENT_DRAG)
	{
	}
	else if (event.id == KU_TABLE_EVENT_KEY_DOWN)
	{
	    ku_event_t ev = event.ev;

	    if (ev.keycode == XKB_KEY_Down || ev.keycode == XKB_KEY_Up)
	    {
		int32_t index = event.table->selected_index;

		if (ev.keycode == XKB_KEY_Down) index += 1;
		if (ev.keycode == XKB_KEY_Up) index -= 1;
		ku_table_select(event.table, index, 0);
	    }
	}
	else if (event.id == KU_TABLE_EVENT_DROP)
	{
	}
    }
    else if (event.table == ui.contexttable)
    {
	if (event.id == KU_TABLE_EVENT_SELECT)
	{
	    if (event.selected_index == 0) ui_open_metadata_editor();
	    if (event.selected_index == 1)
	    {
		if (ui.songtable->selected_items->length > 0)
		{
		    mt_map_t* song = ui.songtable->selected_items->data[0];
		    lib_remove_entry(song);
		    fm_delete_file(config_get("lib_path"), song);
		    lib_write(config_get("lib_path"));
		    ui_update_songlist();
		}
	    }
	    if (event.selected_index == 2)
	    {
		if (ui.songtable->selected_items->length > 0)
		{
		    mt_map_t* song  = ui.songtable->selected_items->data[0];
		    uint32_t  index = songlist_get_index(song);
		    if (index < UINT32_MAX) ku_table_select(ui.songtable, index, 0);
		}
	    }
	    ku_view_remove_from_parent(ui.contextpopupcont);
	}
    }
    else if (event.table == ui.genretable)
    {
	if (event.id == KU_TABLE_EVENT_SELECT)
	{
	    mt_vector_t* selected = event.selected_items;
	    mt_map_t*    info     = selected->data[0];

	    char* genre = MGET(info, "genre");
	    char  filter[100];

	    snprintf(filter, 100, "genre is %s", genre);
	    songlist_set_filter(filter);
	    ui_update_songlist();

	    vh_textinput_set_text(ui.filtertf, filter);
	}
    }
    else if (event.table == ui.artisttable)
    {
	if (event.id == KU_TABLE_EVENT_SELECT)
	{
	    mt_vector_t* selected = event.selected_items;
	    mt_map_t*    info     = selected->data[0];

	    char* artist = MGET(info, "artist");
	    char  filter[100];

	    snprintf(filter, 100, "artist is %s", artist);
	    songlist_set_filter(filter);
	    ui_update_songlist();

	    vh_textinput_set_text(ui.filtertf, filter);
	}
    }
    else if (event.table == ui.metatable)
    {
	if (event.id == KU_TABLE_EVENT_OPEN)
	{
	    // show input textfield above rowwiew

	    if (event.rowview)
	    {
		// show value in input textfield
		mt_map_t* info = event.selected_items->data[0];
		char*     key  = MGET(info, "key");

		if (strcmp(key, "artist") == 0 ||
		    strcmp(key, "album") == 0 ||
		    strcmp(key, "title") == 0 ||
		    strcmp(key, "genre") == 0)
		{
		    ui.inputmode = UI_IM_EDITING;

		    ku_view_t* valueview = event.rowview->views->data[1];
		    ku_rect_t  rframe    = valueview->frame.global;
		    ku_rect_t  iframe    = ui.inputbck->frame.global;
		    iframe.x             = rframe.x;
		    iframe.y             = rframe.y - 5;
		    ku_view_set_frame(ui.inputbck, iframe);

		    ku_view_add_subview(ui.view_base, ui.inputarea);

		    ku_view_layout(ui.view_base);

		    ku_window_activate(ui.window, ui.inputtf);
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
	}
    }
}

/* creates a table from layer structure */
/* TODO move this maybe to ku_gen_type? */

ku_table_t* ui_create_table(ku_view_t* view, mt_vector_t* fields)
{
    /* <div id="filetable" class="colflex marginlt4"> */
    /*   <div id="filetablehead" class="tablehead overflowhidden"> */
    /*       <div id="tiletableheadrow" class="head rowtext"/> */
    /*   </div> */
    /*   <div id="filetablelayers" class="fullscaleview overflowhidden"> */
    /*     <div id="filetablebody" class="tablebody fullscaleview"> */
    /* 	     <div id="filetable_row_a" class="rowa" type="label" text="row a"/> */
    /* 	     <div id="filetable_row_b" class="rowb" type="label" text="row b"/> */
    /* 	     <div id="filetable_row_selected" class="rowselected" type="label" text="row selected"/> */
    /*     </div> */
    /*     <div id="filetablescroll" class="fullscaleview"> */
    /* 	     <div id="filetablevertscroll" class="vertscroll"/> */
    /* 	     <div id="filetablehoriscroll" class="horiscroll"/> */
    /*     </div> */
    /*     <div id="filetableevt" class="fullscaleview"/> */
    /*   </div> */
    /* </div> */

    ku_view_t* header  = NULL;
    ku_view_t* headrow = NULL;
    ku_view_t* layers  = NULL;
    ku_view_t* body    = NULL;
    ku_view_t* scroll  = NULL;
    ku_view_t* event   = NULL;
    ku_view_t* row_a   = NULL;
    ku_view_t* row_b   = NULL;
    ku_view_t* row_s   = NULL;

    if (view->views->length == 2)
    {
	/* header view is present */
	header = view->views->data[0];
	layers = view->views->data[1];
	if (header->views->length > 0)
	{
	    if (layers->views->length == 3)
	    {
		headrow = header->views->data[0];
		body    = layers->views->data[0];
		scroll  = layers->views->data[1];
		event   = layers->views->data[2];

		if (body->views->length == 3)
		{
		    row_a = body->views->data[0];
		    row_b = body->views->data[1];
		    row_s = body->views->data[2];
		}
		else mt_log_error("Missing row a, row b or row selected views for %s\n", view->id);
	    }
	    else mt_log_error("Missing body, scroll and event layers for %s", view->id);
	}
	else mt_log_error("Missing header row view for %s", view->id);
    }
    else if (view->views->length == 1)
    {
	/* no header */
	layers = view->views->data[0];
	if (layers->views->length == 3)
	{
	    body   = layers->views->data[0];
	    scroll = layers->views->data[1];
	    event  = layers->views->data[2];
	    if (body->views->length == 3)
	    {
		row_a = body->views->data[0];
		row_b = body->views->data[1];
		row_s = body->views->data[2];
	    }
	    else mt_log_error("Missing row a, row b or row selected views for %s\n", view->id);
	}
	else if (layers->views->length == 2)
	{
	    body  = layers->views->data[0];
	    event = layers->views->data[1];
	    if (body->views->length == 3)
	    {
		row_a = body->views->data[0];
		row_b = body->views->data[1];
		row_s = body->views->data[2];
	    }
	    else mt_log_error("Missing row a, row b or row selected views for %s\n", view->id);
	}
	else mt_log_error("Missing body, scroll and event layers for %s", view->id);
    }
    else mt_log_error("Missing layers for %s", view->id);

    if (row_a)
    {
	textstyle_t rowastyle = ku_gen_textstyle_parse(row_a);
	textstyle_t rowbstyle = ku_gen_textstyle_parse(row_b);
	textstyle_t rowsstyle = ku_gen_textstyle_parse(row_s);
	textstyle_t headstyle = headrow == NULL ? (textstyle_t){0} : ku_gen_textstyle_parse(headrow);

	ku_table_t* table = ku_table_create(
	    view->id,
	    body,
	    scroll,
	    event,
	    header,
	    fields,
	    rowastyle,
	    rowbstyle,
	    rowsstyle,
	    headstyle,
	    on_table_event);

	/* rows are not needed any more */

	ku_view_remove_from_parent(row_a);
	ku_view_remove_from_parent(row_b);
	ku_view_remove_from_parent(row_s);

	if (headrow) ku_view_remove_from_parent(headrow);

	return table;
    }

    return NULL;
}

void ui_init(int width, int height, float scale, ku_window_t* window)
{
    ku_text_init();

    ui.window = window;

    ui.edited_changed = MNEW();
    ui.edited_deleted = VNEW();

    /* generate views from descriptors */

    mt_vector_t* view_list = VNEW();

    ku_gen_html_parse(config_get("html_path"), view_list);
    ku_gen_css_apply(view_list, config_get("css_path"), config_get("res_path"), 1.0);
    ku_gen_type_apply(view_list, ui_on_btn_event, NULL);

    ku_view_t* bv = mt_vector_head(view_list);

    ui.view_base = RET(bv);
    ku_window_add(ui.window, ui.view_base);

    REL(view_list);

    /* listen for keys and shortcuts */

    vh_key_add(ui.view_base, ui_on_key_down);
    ui.view_base->needs_key = 1;

    /* knobs */

    ku_view_t* seekknob = ku_view_get_subview(ui.view_base, "seekknob");
    ku_view_t* volknob  = ku_view_get_subview(ui.view_base, "volknob");

    tg_knob_add(seekknob);
    vh_knob_add(seekknob, ui_pos_change);

    tg_knob_add(volknob);
    vh_knob_add(volknob, ui_vol_change);

    ui.volknob  = volknob;
    ui.seekknob = seekknob;

    /* setup volume */

    ui.volume = 0.8;
    tg_knob_set_angle(ui.volknob, ui.volume * 6.28 - 3.14 / 2.0);

    /* textfields */

    ui.infotf = ku_view_get_subview(ui.view_base, "infotf");

    ui.filtertf = ku_view_get_subview(ui.view_base, "filtertf");

    vh_textinput_add(ui.filtertf, "", "Filter", ui_on_text_event);

    ui.timetf = ku_view_get_subview(ui.view_base, "timetf");

    ui.inputarea = RET(ku_view_get_subview(ui.view_base, "inputarea"));
    ui.inputbck  = ku_view_get_subview(ui.view_base, "inputbck");
    ui.inputtf   = ku_view_get_subview(ui.view_base, "inputtf");

    ui.inputbck->blocks_touch   = 1;
    ui.inputarea->blocks_touch  = 1;
    ui.inputarea->blocks_scroll = 1;

    vh_textinput_add(ui.inputtf, "Generic input", "", ui_on_text_event);

    vh_touch_add(ui.inputarea, ui_on_touch);

    ku_view_remove_from_parent(ui.inputarea);

    /* songlist */

    char*        fieldconfig = config_get("fields");
    mt_vector_t* words       = mt_string_split(fieldconfig, " ");
    mt_vector_t* fields      = VNEW();

    for (int index = 0; index < words->length; index += 2)
    {
	char* field = words->data[index];
	char* value = words->data[index + 1];

	printf("field %s %s\n", field, value);

	VADD(fields, field);
	VADDR(fields, mt_number_new_int(atoi(value)));
    }

    REL(words);

    ui.songtable = ui_create_table(GETV(bv, "songtable"), fields);

    REL(fields);

    ku_window_activate(ui.window, GETV(bv, "songtableevt"));

    /* settings list */

    ku_view_t* settingspopupcont = ku_view_get_subview(ui.view_base, "settingspopupcont");
    ku_view_t* settingspopup     = ku_view_get_subview(ui.view_base, "settingspopup");

    settingspopup->blocks_touch  = 1;
    settingspopup->blocks_scroll = 1;

    ui.settingspopupcont = RET(settingspopupcont);

    fields = VNEW();

    VADDR(fields, mt_string_new_cstring("value"));
    VADDR(fields, mt_number_new_int(510));

    ui.settingstable = ui_create_table(GETV(bv, "settingstable"), fields);

    REL(fields);

    mt_vector_t* items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Visual Music Player v%s beta by Milan Toth", VMP_VERSION)}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Free and Open Source Software")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Donate on Paypal")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Organize Library : %s", config_get("lib_organize"))}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Library Path : %s", config_get("lib_path"))}));

    ku_table_set_data(ui.settingstable, items);
    REL(items);

    ku_view_remove_from_parent(ui.settingspopupcont);

    /* genre and artist lists */

    ku_view_t* filterpopupcont = ku_view_get_subview(ui.view_base, "filterpopupcont");
    ku_view_t* filterpopup     = ku_view_get_subview(ui.view_base, "filterpopup");

    filterpopup->blocks_touch  = 1;
    filterpopup->blocks_scroll = 1;

    ui.filterpopupcont = RET(filterpopupcont);

    mt_vector_t* genrefields  = VNEW();
    mt_vector_t* artistfields = VNEW();

    VADDR(genrefields, mt_string_new_cstring("genre"));
    VADDR(genrefields, mt_number_new_int(150));

    VADDR(artistfields, mt_string_new_cstring("artist"));
    VADDR(artistfields, mt_number_new_int(350));

    ui.genretable  = ui_create_table(GETV(bv, "genretable"), genrefields);
    ui.artisttable = ui_create_table(GETV(bv, "artisttable"), artistfields);

    REL(genrefields);
    REL(artistfields);

    ku_view_remove_from_parent(ui.filterpopupcont);

    /* song metadata */

    ku_view_t* metapopupcont = ku_view_get_subview(ui.view_base, "metapopupcont");
    ku_view_t* metapopup     = ku_view_get_subview(ui.view_base, "metapopup");
    ku_view_t* metaacceptbtn = ku_view_get_subview(ui.view_base, "metaacceptbtn");

    ui.metashadow    = ku_view_get_subview(ui.view_base, "metashadow");
    ui.metaacceptbtn = RET(metaacceptbtn);

    ku_view_remove_from_parent(ui.metaacceptbtn);

    metapopup->blocks_touch  = 1;
    metapopup->blocks_scroll = 1;

    ui.metapopupcont = RET(metapopupcont);

    mt_vector_t* metafields = VNEW();

    VADDR(metafields, mt_string_new_cstring("key"));
    VADDR(metafields, mt_number_new_int(100));
    VADDR(metafields, mt_string_new_cstring("value"));
    VADDR(metafields, mt_number_new_int(350));

    ui.metatable = ui_create_table(GETV(bv, "metatable"), metafields);

    REL(metafields);

    ku_view_remove_from_parent(ui.metapopupcont);

    /* context list */

    ku_view_t* contextpopupcont = ku_view_get_subview(ui.view_base, "contextpopupcont");
    ku_view_t* contextpopup     = ku_view_get_subview(ui.view_base, "contextpopup");

    contextpopup->blocks_touch  = 1;
    contextpopup->blocks_scroll = 1;

    ui.contextpopupcont = RET(contextpopupcont);

    fields = VNEW();

    VADDR(fields, mt_string_new_cstring("value"));
    VADDR(fields, mt_number_new_int(200));

    ui.contexttable = ui_create_table(GETV(bv, "contexttable"), fields);

    REL(fields);

    items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Edit song")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Delete song")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Go to current")}));

    ku_table_set_data(ui.contexttable, items);
    REL(items);

    vh_touch_add(ui.contextpopupcont, ui_on_touch);

    ku_view_remove_from_parent(ui.contextpopupcont);

    /* get visual views */

    ui.cover       = ku_view_get_subview(ui.view_base, "cover");
    ui.visL        = ku_view_get_subview(ui.view_base, "visL");
    ui.visR        = ku_view_get_subview(ui.view_base, "visR");
    ui.visuals     = RET(ku_view_get_subview(ui.view_base, "visuals"));
    ui.songlisttop = RET(ku_view_get_subview(ui.view_base, "songlisttop"));

    ku_view_gen_texture(ui.visL);
    ku_view_gen_texture(ui.visR);
    ku_draw_rect(ui.visL->texture.bitmap, 0, ui.visL->texture.bitmap->h / 2, ui.visL->texture.bitmap->w, 1, 0xFFFFFFFF, 1);
    ui.visL->texture.changed = 1;
    ku_draw_rect(ui.visR->texture.bitmap, 0, ui.visR->texture.bitmap->h / 2, ui.visR->texture.bitmap->w, 1, 0xFFFFFFFF, 1);
    ui.visR->texture.changed = 1;

    ku_view_gen_texture(ui.visL);
    ku_view_gen_texture(ui.visR);
    ku_view_gen_texture(ui.cover);

    vh_touch_add(ui.visL, ui_on_touch);
    vh_touch_add(ui.visR, ui_on_touch);

    // show texture map for debug

    /* ku_view_t* texmap       = ku_view_new("texmap", ((ku_rect_t){0, 0, 150, 150})); */
    /* texmap->needs_touch  = 0; */
    /* texmap->exclude      = 0; */
    /* texmap->texture.full = 1; */
    /* texmap->style.right  = 0; */
    /* texmap->style.top    = 0; */

    /* ku_window_add(ui.window,texmap); */
}

void ui_destroy()
{
    ku_window_remove(ui.window, ui.view_base);

    if (ui.played_song) REL(ui.played_song);

    REL(ui.visuals);
    REL(ui.settingspopupcont);
    REL(ui.songlisttop);
    REL(ui.metapopupcont);
    REL(ui.filterpopupcont);
    REL(ui.contextpopupcont);

    REL(ui.view_base);

    REL(ui.metatable);
    REL(ui.songtable);
    REL(ui.contexttable);
    REL(ui.settingstable);
    REL(ui.artisttable);
    REL(ui.genretable);

    REL(ui.metaacceptbtn);
    REL(ui.inputarea);

    REL(ui.edited_changed);
    REL(ui.edited_deleted);

    ku_text_destroy(); // DESTROY 0
}

void ui_play_next()
{
    mt_map_t* next = NULL;

    if (ui.shuffle == 0 && ui.played_song) next = songlist_get_next_song(ui.played_song);
    else next = songlist_get_song(1);

    ui_play_song(next);
}

void ui_play_prev()
{
    mt_map_t* prev = NULL;

    if (ui.shuffle == 0 && ui.played_song) prev = songlist_get_prev_song(ui.played_song);
    else prev = songlist_get_song(1);

    ui_play_song(prev);
}

void ui_toggle_pause()
{
    if (ui.ms)
    {
	ku_view_t* playbtn = ku_view_get_subview(ui.view_base, "playbtn");

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
    ui.cursorv                         = ku_view_new("ui.cursor", ((ku_rect_t){10, 10, 10, 10}));
    ui.cursorv->style.background_color = 0xFF000099;
    ui.cursorv->needs_touch            = 0;
    tg_css_add(ui.cursorv);
    ku_window_add(ui.window, ui.cursorv);
}

/* updates cursor position */

void ui_update_cursor(ku_rect_t frame)
{
    ku_view_set_frame(ui.cursorv, frame);
}

void ui_update_player()
{
    if (ui.ms)
    {
	if (!ui.ms->paused)
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
		tg_text_set1(ui.timetf, timebuff);

		double ratio = time / ui.ms->duration;
		tg_knob_set_angle(ui.seekknob, ratio * 6.28 - 3.14 / 2.0);
	    }

	    if (ui.ms->finished) ui_play_next();
	}
    }
}

void ui_show_progress(char* progress)
{
    tg_text_set1(ui.infotf, progress);
}

#endif
