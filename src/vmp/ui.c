#ifndef ui_h
#define ui_h

#include "ku_connector_wayland.c"
#include "ku_view.c"
#include "ku_window.c"

void ui_init(int width, int height, float scale, ku_window_t* window, wl_window_t* wlwindow);
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
#include "ku_draw.c"
#include "ku_fontconfig.c"
#include "ku_gen_css.c"
#include "ku_gen_html.c"
#include "ku_gen_textstyle.c"
#include "ku_gen_type.c"
#include "ku_text.c"
#include "library.c"
#include "mediaplayer.c"
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
#include "vh_anim.c"
#include "vh_button.c"
#include "vh_key.c"
#include "vh_knob.c"
#include "vh_table.c"
#include "vh_textinput.c"
#include "vh_touch.c"
#include <xkbcommon/xkbcommon.h>

enum _ui_inputmode
{
    UI_IM_SORTING,
    UI_IM_EDITING,
    UI_IM_COVERART
};
typedef enum _ui_inputmode ui_inputmode;

struct _ui_t
{
    ku_window_t* window; /* window for this ui */
    wl_window_t* wlwindow;

    ku_view_t* songtablev;
    ku_view_t* metatablev;
    ku_view_t* genretablev;
    ku_view_t* artisttablev;

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

    ku_view_t* rowview_for_context_menu; /* TODO do we need this? */

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
	// TODO don't reset songlist, causes jumps on normal play
	/* ui_update_songlist(); */
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
    if (index < UINT32_MAX) vh_table_select(ui.songtablev, index, 0);

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

	vh_table_t* vh = (vh_table_t*) ui.songtablev->evt_han_data;
	if (vh->selected_items->length > 0)
	{
	    mt_map_t* info = vh->selected_items->data[0];

	    ku_view_t* cover    = ku_view_get_subview(ui.metapopupcont, "metacover");
	    char*      path     = MGET(info, "path");
	    char*      realpath = mt_path_new_append(config_get("lib_path"), path);

	    if (!cover->texture.bitmap)
		ku_view_gen_texture(cover);

	    int success = coder_load_cover_into(realpath, cover->texture.bitmap);

	    if (!success)
		ku_view_gen_texture(cover);

	    cover->texture.changed = 1;

	    REL(realpath);

	    mt_vector_t* pairs = VNEW();
	    mt_vector_t* keys  = VNEW();
	    mt_map_keys(info, keys);
	    for (size_t index = 0; index < keys->length; index++)
	    {
		char*     key   = keys->data[index];
		char*     value = MGET(info, key);
		mt_map_t* map   = MNEW();
		MPUT(map, "key", key);
		MPUT(map, "value", value);
		VADDR(pairs, map);
	    }

	    mt_vector_sort(pairs, ui_comp_value);

	    vh_table_set_data(ui.metatablev, pairs);
	    REL(pairs);
	    REL(keys);

	    ku_view_layout(ui.view_base, ui.view_base->style.scale);

	    ui.edited_song = info;
	}

	ku_view_t* metatableevt = GETV(ui.metapopupcont, "metatable_event");
	ku_window_activate(ui.window, metatableevt, 1);
    }
}

void ui_cancel_input()
{
    ku_view_remove_subview(ui.view_base, ui.inputarea);
}

void ui_show_context_menu(int x, int y)
{
    if (ui.contextpopupcont->parent == NULL)
    {
	ku_view_t* contextpopup = ui.contextpopupcont->views->data[0];
	ku_rect_t  iframe       = contextpopup->frame.global;
	iframe.x                = x + 20.0;
	iframe.y                = y;

	if (iframe.y + iframe.h > ui.window->height)
	    iframe.y = ui.window->height - iframe.h;
	if (iframe.x + iframe.w > ui.window->width)
	    iframe.x = ui.window->width - iframe.w;

	ku_view_add_subview(ui.view_base, ui.contextpopupcont);
	ku_view_layout(ui.view_base, ui.view_base->style.scale);
	ku_view_set_frame(contextpopup, iframe);

	ku_view_t* contexttableevt = GETV(ui.contextpopupcont, "contexttable_event");
	ku_window_activate(ui.window, contexttableevt, 1);

	ku_rect_t start = contextpopup->frame.local;

	ku_rect_t end = start;

	//start.x += 40;
	//start.w -= 80;
	start.h = 10;
	start.y -= 5;
	ku_view_set_frame(contextpopup, start);

	vh_anim_frame(contextpopup, start, end, 0, 20, AT_EASE);

	ku_view_t* contextanim = GETV(contextpopup, "contextpopupanim");

	start = contextanim->frame.local;
	end   = start;

	//start.w -= 80;
	start.h = 10;
	start.y -= 5;
	ku_view_set_frame(contextanim, start);

	vh_anim_frame(contextanim, start, end, 0, 20, AT_EASE);
    }
}

void ui_on_key_down(vh_key_event_t event)
{
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
	    if (ui.ms)
		mp_play(ui.ms);
	    else
	    {
		mt_map_t* song = songlist_get_song(0);
		if (song)
		    ui_play_song(song);
	    }
	}
	else
	{
	    if (ui.ms)
		mp_pause(ui.ms);
	}
    };
    if (strcmp(event.view->id, "mutebtn") == 0)
    {
	if (ui.ms)
	{
	    if (event.vh->state == VH_BUTTON_DOWN)
		mp_mute(ui.ms);
	    else
		mp_unmute(ui.ms);
	}
    };
    if (strcmp(event.view->id, "prevbtn") == 0)
    {
	mt_map_t* prev = NULL;

	if (ui.shuffle == 0 && ui.played_song)
	    prev = songlist_get_prev_song(ui.played_song);
	else
	    prev = songlist_get_song(1);

	if (prev)
	    ui_play_song(prev);
    };
    if (strcmp(event.view->id, "nextbtn") == 0)
    {
	mt_map_t* next = NULL;

	if (ui.shuffle == 0 && ui.played_song)
	    next = songlist_get_next_song(ui.played_song);
	else
	    next = songlist_get_song(1);

	if (next)
	    ui_play_song(next);
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
	    ku_view_layout(ui.view_base, ui.view_base->style.scale);
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
	ku_view_layout(ui.view_base, ui.view_base->style.scale);
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

	    vh_table_set_data(ui.genretablev, genres);
	    vh_table_set_data(ui.artisttablev, artists);

	    REL(genres);
	    REL(artists);

	    ku_view_layout(ui.view_base, ui.view_base->style.scale);

	    /* ku_window_activate(ui.window, ui.filtertf, 1); */
	    /* vh_textinput_activate(ui.filtertf, 1); */
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
	ku_view_layout(ui.view_base, ui.view_base->style.scale);

	ku_window_activate(ui.window, ui.inputtf, 1);
	vh_textinput_activate(ui.inputtf, 1);

	vh_textinput_set_text(ui.inputtf, config_get("sorting"));

	ui.inputmode = UI_IM_SORTING;
    };
    if (strcmp(event.view->id, "clearbtn") == 0)
    {
	vh_textinput_set_text(ui.filtertf, "");
	/* ku_window_activate(ui.window, ui.filtertf, 1); */
	/* vh_textinput_activate(ui.filtertf, 1); */

	songlist_set_filter(NULL);
	ui_update_songlist();
    };
    if (strcmp(event.view->id, "inputclearbtn") == 0)
    {
	vh_textinput_set_text(ui.inputtf, "");
	ku_window_activate(ui.window, ui.inputtf, 1);
	vh_textinput_activate(ui.inputtf, 1);
    };
    if (strcmp(event.view->id, "exitbtn") == 0)
	ku_wayland_exit();
    if (strcmp(event.view->id, "maxbtn") == 0)
	ku_wayland_toggle_fullscreen(ui.wlwindow);
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
	mt_memory_describe(ui.edited_changed, 0);

	char* path   = MGET(ui.edited_song, "path");
	int   result = coder_write_metadata(config_get("lib_path"), path, ui.edited_cover, ui.edited_changed, ui.edited_deleted);
	if (result >= 0)
	{
	    printf("metadata udated\n");
	    // modify song in db also if metadata is successfully written into file
	    lib_update_metadata(path, ui.edited_changed, ui.edited_deleted);
	    if (config_get_bool("lib_organize"))
		lib_organize_entry(config_get("lib_path"), lib_get_db(), ui.edited_song);
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
	ku_view_layout(ui.view_base, ui.view_base->style.scale);

	ku_window_activate(ui.window, ui.inputtf, 1);
	vh_textinput_activate(ui.inputtf, 1);

	vh_textinput_set_text(ui.inputtf, "/home/path_to_image/image.png");

	ui.inputmode = UI_IM_COVERART;
    }
}

void ui_on_text_event(vh_textinput_event_t event)
{
    if (strcmp(event.view->id, "filtertf") == 0)
    {
	if (event.id == VH_TEXTINPUT_DEACTIVATE || event.id == VH_TEXTINPUT_RETURN)
	{
	    vh_textinput_activate(event.view, 0);
	    ku_window_activate(ui.window, event.view, 0);
	}

	songlist_set_filter(event.text);
	ui_update_songlist();
    }
    else if (strcmp(event.view->id, "inputtf") == 0)
    {
	if (event.id == VH_TEXTINPUT_DEACTIVATE)
	    ui_cancel_input();

	if (event.id == VH_TEXTINPUT_RETURN)
	{
	    if (ui.inputmode == UI_IM_EDITING)
	    {
		printf("EDITING ENDED, %s %s\n", ui.edited_key, event.text);

		// save value of inputtf

		ku_view_add_subview(ui.metashadow, ui.metaacceptbtn);
		ui_cancel_input();

		MPUTR(ui.edited_song, ui.edited_key, STRNC(event.text));
		MPUTR(ui.edited_changed, ui.edited_key, STRNC(event.text));

		mt_vector_t* pairs = VNEW();
		mt_vector_t* keys  = VNEW();
		mt_map_keys(ui.edited_song, keys);

		for (size_t index = 0; index < keys->length; index++)
		{
		    char*     key   = keys->data[index];
		    char*     value = MGET(ui.edited_song, key);
		    mt_map_t* map   = MNEW();
		    MPUT(map, "key", key);
		    MPUT(map, "value", value);
		    VADDR(pairs, map);
		}

		mt_vector_sort(pairs, ui_comp_value);

		vh_table_set_data(ui.metatablev, pairs);

		REL(pairs);
		REL(keys);
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
		char* path_final = mt_path_new_append(config_get("wrk_path"), event.text); // REL 1
		// check if image is valid
		ku_bitmap_t* image = coder_load_image(path_final);

		printf("PATH %s\n", path_final);

		if (image)
		{
		    printf("loading image\n");
		    ku_view_t* cover = ku_view_get_subview(ui.metapopupcont, "metacover");

		    if (!cover->texture.bitmap)
			ku_view_gen_texture(cover);

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

    if (ui.ms) mp_set_position(ui.ms, ratio);
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
    if (ui.ms) mp_set_volume(ui.ms, ui.volume);
}

void ui_update_songlist()
{
    vh_table_set_data(ui.songtablev, songlist_get_visible_songs());
}

void on_table_event(vh_table_event_t event)
{
    if (strcmp(event.view->id, "songtable") == 0)
    {
	if (event.id == VH_TABLE_EVENT_FIELDS_UPDATE)
	{
	    mt_vector_t* fields   = event.fields;
	    char*        fieldstr = mt_string_new_cstring("");
	    for (size_t index = 0; index < fields->length; index += 2)
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
	else if (event.id == VH_TABLE_EVENT_FIELD_SELECT)
	{
	    char* field      = event.field;
	    char* sorting    = mt_string_new_cstring(songlist_get_sorting());
	    char* newsorting = NULL;

	    if (strstr(sorting, field) != NULL)
	    {
		char* part  = strstr(sorting, field);
		char  value = part[strlen(field) + 1];

		if (strcmp(field, "artist") == 0)
		    newsorting = STRNF(100, "%s %c album 1 track 1", field, value == '0' ? '1' : '0');
		else
		    newsorting = STRNF(100, "%s %c", field, value == '0' ? '1' : '0');
	    }
	    else
	    {
		if (strcmp(field, "artist") == 0)
		    newsorting = STRNF(100, "%s 1 album 1 track 1", field);
		else
		    newsorting = STRNF(100, "%s 1", field);
	    }

	    if (newsorting)
	    {
		REL(sorting);
		sorting = newsorting;
	    }

	    songlist_set_sorting(sorting);

	    REL(sorting);

	    config_set("sorting", sorting);
	    config_write(config_get("cfg_path"));

	    ui_update_songlist();
	}
	else if (event.id == VH_TABLE_EVENT_CONTEXT)
	{
	    ui_show_context_menu(event.ev.x, event.ev.y);
	}
	else if (event.id == VH_TABLE_EVENT_SELECT && event.ev.repeat == 0)
	{
	    ui.rowview_for_context_menu = event.rowview;
	}
	else if (event.id == VH_TABLE_EVENT_OPEN)
	{
	    mt_vector_t* selected = event.selected_items;
	    mt_map_t*    info     = selected->data[0];

	    ui_play_song(info);
	}
	else if (event.id == VH_TABLE_EVENT_KEY_UP)
	{
	    if (event.ev.keycode == XKB_KEY_space)
		ui_toggle_pause();

	    if (event.ev.keycode == XKB_KEY_v && event.ev.ctrl_down)
	    {
		if (ui.rowview_for_context_menu)
		{
		    ku_rect_t frame = ui.rowview_for_context_menu->frame.global;
		    ui_show_context_menu(frame.x, frame.y);
		}
	    }

	    if (event.ev.keycode == XKB_KEY_f && event.ev.ctrl_down)
	    {
		ku_window_activate(ui.window, ui.filtertf, 1);
		vh_textinput_activate(ui.filtertf, 1);
	    }
	}
    }
    else if (strcmp(event.view->id, "contexttable") == 0)
    {
	if (event.id == VH_TABLE_EVENT_OPEN || (event.id == VH_TABLE_EVENT_SELECT && event.ev.type == KU_EVENT_MOUSE_DOWN))
	{
	    if (event.selected_index == 0)
		ui_open_metadata_editor();
	    if (event.selected_index == 1)
	    {
		vh_table_t* vh = (vh_table_t*) ui.songtablev->evt_han_data;

		if (vh->selected_items->length > 0)
		{
		    mt_map_t* song = vh->selected_items->data[0];
		    lib_remove_entry(song);
		    fm_delete_file(config_get("lib_path"), song);
		    lib_write(config_get("lib_path"));

		    mt_vector_t* entries = VNEW();
		    lib_get_entries(entries);
		    songlist_set_songs(entries);
		    REL(entries);

		    ui_update_songlist();
		}
	    }
	    if (event.selected_index == 2)
	    {
		if (ui.played_song)
		{
		    uint32_t index = songlist_get_index(ui.played_song);
		    if (index < UINT32_MAX)
			vh_table_select(ui.songtablev, index, 0);
		}
	    }
	    ku_view_t* contexttableevt = GETV(ui.contextpopupcont, "contexttable_event");
	    ku_window_activate(ui.window, contexttableevt, 0);
	    ku_view_remove_from_parent(ui.contextpopupcont);
	}
	else if (event.id == VH_TABLE_EVENT_KEY_UP)
	{
	    if (event.ev.keycode == XKB_KEY_Escape)
	    {
		ku_view_t* contexttableevt = GETV(ui.contextpopupcont, "contexttable_event");
		ku_window_activate(ui.window, contexttableevt, 0);
		ku_view_remove_from_parent(ui.contextpopupcont);
	    }
	}
    }
    else if (strcmp(event.view->id, "genretable") == 0)
    {
	if (event.id == VH_TABLE_EVENT_SELECT)
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
    else if (strcmp(event.view->id, "artisttable") == 0)
    {
	if (event.id == VH_TABLE_EVENT_SELECT)
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
    else if (strcmp(event.view->id, "metatable") == 0)
    {
	if (event.id == VH_TABLE_EVENT_OPEN)
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

		    ku_view_layout(ui.view_base, ui.view_base->style.scale);

		    ku_window_activate(ui.window, ui.inputtf, 1);
		    vh_textinput_activate(ui.inputtf, 1);

		    char* value = MGET(info, "value");
		    if (!value)
			value = "";

		    if (value)
		    {
			vh_textinput_set_text(ui.inputtf, value);

			ui.edited_key = MGET(info, "key");
		    }
		}
	    }
	}
	else if (event.id == VH_TABLE_EVENT_KEY_UP)
	{
	    if (event.ev.keycode == XKB_KEY_Escape)
	    {
		ku_view_t* metatableevt = GETV(ui.metapopupcont, "metatable_event");
		ku_window_activate(ui.window, metatableevt, 0);
		ku_view_remove_from_parent(ui.metapopupcont);
	    }
	}
    }
    else if (strcmp(event.view->id, "settingstable") == 0)
    {
	if (event.id == VH_TABLE_EVENT_SELECT)
	{
	    if (event.selected_index == 2)
	    {
		char newurl[100];
		snprintf(newurl, 100, "xdg-open https://paypal.me/milgra");
		int result = system(newurl);
		if (result < 0)
		    mt_log_error("system call error %s", newurl);
		else
		    ui_show_progress("Link opened in the browser");
	    }
	    if (event.selected_index == 4)
	    {
		ui_show_progress("Set organization with -o command line parameter");
	    }
	    if (event.selected_index == 5)
	    {
		ui_show_progress("Set library path with -l command line parameter");
	    }
	}
    }
}

void ui_init(int width, int height, float scale, ku_window_t* window, wl_window_t* wlwindow)
{
    ku_text_init();

    ui.window   = window;
    ui.wlwindow = wlwindow;

    ui.edited_changed = MNEW();
    ui.edited_deleted = VNEW();

    /* generate views from descriptors */

    mt_vector_t* view_list = VNEW();

    ku_gen_html_parse(config_get("html_path"), view_list);
    ku_gen_css_apply(view_list, config_get("css_path"), config_get("img_path"));
    ku_gen_type_apply(view_list, ui_on_btn_event, NULL);

    ku_view_t* bv = mt_vector_head(view_list);

    ui.view_base = RET(bv);
    ku_window_add(ui.window, ui.view_base);

    REL(view_list);

    /* listen for keys and shortcuts */

    vh_key_add(ui.view_base, ui_on_key_down);
    ku_window_activate(ui.window, ui.view_base, 1);

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

    vh_textinput_add(ui.inputtf, "Generic input", "", ui_on_text_event);

    vh_touch_add(ui.inputarea, ui_on_touch);

    ku_view_remove_from_parent(ui.inputarea);

    /* songlist */

    char*        fieldconfig = config_get("fields");
    mt_vector_t* words       = mt_string_split(fieldconfig, " ");
    mt_vector_t* fields      = VNEW();

    for (size_t index = 0; index < words->length; index += 2)
    {
	char* field = words->data[index];
	char* value = words->data[index + 1];

	VADD(fields, field);
	VADDR(fields, mt_number_new_int(atoi(value)));
    }

    REL(words);

    ui.songtablev = GETV(bv, "songtable");

    vh_table_attach(ui.songtablev, fields, on_table_event);

    REL(fields);

    vh_table_t* vh = ui.songtablev->evt_han_data;
    ku_window_activate(ui.window, vh->evnt_v, 1);

    /* settings list */

    ku_view_t* settingspopupcont = ku_view_get_subview(ui.view_base, "settingspopupcont");

    ui.settingspopupcont = RET(settingspopupcont);

    fields = VNEW();

    VADDR(fields, mt_string_new_cstring("value"));
    VADDR(fields, mt_number_new_int(510));

    ku_view_t* settingstablev = GETV(bv, "settingstable");
    vh_table_attach(settingstablev, fields, on_table_event);
    /* vh_table_show_scrollbar(settingstablev, 0); */
    vh_table_show_selected(settingstablev, 0);

    REL(fields);

    mt_vector_t* items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Visual Music Player v%s beta by Milan Toth", VMP_VERSION)}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Free and Open Source Software")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Donate on Paypal")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Organize Library : %s", config_get("lib_organize"))}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Library Path : %s", config_get("lib_path"))}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Context menu/Paste files : CTRL+V")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Filter/Find : CTRL+F")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Cancel popup : ESC")}));

    vh_table_set_data(settingstablev, items);
    REL(items);

    ku_view_remove_from_parent(ui.settingspopupcont);

    /* genre and artist lists */

    ku_view_t* filterpopupcont = ku_view_get_subview(ui.view_base, "filterpopupcont");

    ui.filterpopupcont = RET(filterpopupcont);

    mt_vector_t* genrefields  = VNEW();
    mt_vector_t* artistfields = VNEW();

    VADDR(genrefields, mt_string_new_cstring("genre"));
    VADDR(genrefields, mt_number_new_int(150));

    VADDR(artistfields, mt_string_new_cstring("artist"));
    VADDR(artistfields, mt_number_new_int(350));

    ui.genretablev  = GETV(bv, "genretable");
    ui.artisttablev = GETV(bv, "artisttable");

    vh_table_attach(ui.genretablev, genrefields, on_table_event);
    vh_table_attach(ui.artisttablev, artistfields, on_table_event);

    REL(genrefields);
    REL(artistfields);

    ku_view_remove_from_parent(ui.filterpopupcont);

    /* song metadata */

    ku_view_t* metapopupcont = ku_view_get_subview(ui.view_base, "metapopupcont");
    ku_view_t* metaacceptbtn = ku_view_get_subview(ui.view_base, "metaacceptbtn");

    ui.metashadow    = ku_view_get_subview(ui.view_base, "metashadow");
    ui.metaacceptbtn = RET(metaacceptbtn);

    ku_view_remove_from_parent(ui.metaacceptbtn);

    ui.metapopupcont = RET(metapopupcont);

    mt_vector_t* metafields = VNEW();

    VADDR(metafields, mt_string_new_cstring("key"));
    VADDR(metafields, mt_number_new_int(100));
    VADDR(metafields, mt_string_new_cstring("value"));
    VADDR(metafields, mt_number_new_int(350));

    ui.metatablev = GETV(bv, "metatable");
    vh_table_attach(ui.metatablev, metafields, on_table_event);

    REL(metafields);

    ku_view_remove_from_parent(ui.metapopupcont);

    /* context list */

    ku_view_t* contextpopupcont = ku_view_get_subview(ui.view_base, "contextpopupcont");
    ku_view_t* contextpopup     = ku_view_get_subview(ui.view_base, "contextpopup");
    ku_view_t* contextanimv     = GETV(bv, "contextpopupanim");

    vh_anim_add(contextanimv, NULL, NULL);

    ui.contextpopupcont = RET(contextpopupcont);

    vh_anim_add(contextpopup, NULL, NULL);

    fields = VNEW();

    VADDR(fields, mt_string_new_cstring("value"));
    VADDR(fields, mt_number_new_int(200));

    /* TODO re-think the popup animation, too much stuff is needed to set it up */
    ku_view_t* contexttablev = GETV(bv, "contexttable");
    vh_table_attach(contexttablev, fields, on_table_event);
    vh_table_show_scrollbar(contexttablev, 0);

    vh_table_t* table = contexttablev->evt_han_data;

    /* hack for context menu popup animation */
    table->layr_v->style.masked = 0;

    REL(fields);

    items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Edit song")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Delete song")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Go to current")}));

    vh_table_set_data(contexttablev, items);
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

    REL(ui.metaacceptbtn);
    REL(ui.inputarea);

    REL(ui.edited_changed);
    REL(ui.edited_deleted);

    songlist_destroy();

    ku_fontconfig_delete();

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
	    if (playbtn)
		vh_button_set_state(playbtn, VH_BUTTON_UP);
	}
	else
	{
	    mp_play(ui.ms);
	    if (playbtn)
		vh_button_set_state(playbtn, VH_BUTTON_DOWN);
	}
    }
}

void ui_add_cursor()
{
    ui.cursorv                         = ku_view_new("ui.cursor", ((ku_rect_t){10, 10, 10, 10}));
    ui.cursorv->style.background_color = 0xFF000099;
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
	    double rem;
	    mp_video_refresh(ui.ms, &rem, ui.cover->texture.bitmap, !ui.hide_visuals);

	    if (ui.hide_visuals == 0)
	    {
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
	}
	if (ui.ms->finished)
	    ui_play_next();
    }
}

void ui_show_progress(char* progress)
{
    tg_text_set1(ui.infotf, progress);
}

#endif
