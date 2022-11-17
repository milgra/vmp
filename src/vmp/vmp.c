#include "analyzer.c"
#include "coder.c"
#include "config.c"
#include "evrecorder.c"
#include "filemanager.c"
#include "ku_connector_wayland.c"
#include "ku_gl.c"
#include "ku_renderer_egl.c"
#include "ku_renderer_soft.c"
#include "ku_window.c"
#include "library.c"
#include "mt_bitmap_ext.c"
#include "mt_log.c"
#include "mt_map.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_time.c"
#include "remote.c"
#include "songlist.c"
#include "ui.c"
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct
{
    char              replay;
    char              record;
    struct wl_window* wlwindow;
    ku_window_t*      kuwindow;

    ku_rect_t    dirtyrect;
    int          softrender;
    mt_vector_t* eventqueue;

    float       analyzer_ratio;
    analyzer_t* analyzer;

    int      frames;
    remote_t remote;

    char* rec_path;
    char* rep_path;
} vmp = {0};

void init(wl_event_t event)
{
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_AUDIO);

    vmp.eventqueue = VNEW();

    struct monitor_info* monitor = event.monitors[0];

    if (vmp.softrender)
    {
	vmp.wlwindow = ku_wayland_create_window("vmp", 1200, 600);
    }
    else
    {
	vmp.wlwindow = ku_wayland_create_eglwindow("vmp", 1200, 600);

	int max_width  = 0;
	int max_height = 0;

	for (int index = 0; index < event.monitor_count; index++)
	{
	    struct monitor_info* monitor = event.monitors[index];
	    if (monitor->logical_width > max_width) max_width = monitor->logical_width;
	    if (monitor->logical_height > max_height) max_height = monitor->logical_height;
	}
	ku_renderer_egl_init(max_width, max_height);
    }

    vmp.kuwindow = ku_window_create(monitor->logical_width, monitor->logical_height);

    mt_time(NULL);
    ui_init(monitor->logical_width, monitor->logical_height, monitor->scale, vmp.kuwindow); // DESTROY 3
    mt_time("ui init");

    if (vmp.record)
    {
	ui_add_cursor();
	evrec_init_recorder(vmp.rec_path); // DESTROY 4
    }

    if (vmp.replay)
    {
	ui_add_cursor();
	evrec_init_player(vmp.rep_path); // DESTROY 5
    }

    mt_map_t* fields = MNEW();

    MPUTR(fields, "artist", STRNC("artist"));
    MPUTR(fields, "album", STRNC("album"));
    MPUTR(fields, "title", STRNC("title"));
    MPUTR(fields, "date", STRNC("date"));
    MPUTR(fields, "genre", STRNC("genre"));
    MPUTR(fields, "track", STRNC("track"));
    MPUTR(fields, "disc", STRNC("disc"));
    MPUTR(fields, "duration", STRNC("duration"));
    MPUTR(fields, "channels", STRNC("channels"));
    MPUTR(fields, "bitrate", STRNC("bitrate"));
    MPUTR(fields, "samplerate", STRNC("samplerate"));
    MPUTR(fields, "plays", STRNC("plays"));
    MPUTR(fields, "skips", STRNC("skips"));
    MPUTR(fields, "added", STRNC("added"));
    MPUTR(fields, "played", STRNC("played"));
    MPUTR(fields, "skipped", STRNC("skipped"));
    MPUTR(fields, "type", STRNC("type"));
    MPUTR(fields, "container", STRNC("container"));

    mt_map_t* numfields = MNEW();

    MPUTR(numfields, "track", STRNC("track"));
    MPUTR(numfields, "disc", STRNC("disc"));
    MPUTR(numfields, "duration", STRNC("duration"));
    MPUTR(numfields, "channels", STRNC("channels"));
    MPUTR(numfields, "bitrate", STRNC("bitrate"));
    MPUTR(numfields, "samplerate", STRNC("samplerate"));
    MPUTR(numfields, "plays", STRNC("plays"));
    MPUTR(numfields, "skips", STRNC("skips"));
    MPUTR(numfields, "added", STRNC("added"));
    MPUTR(numfields, "played", STRNC("played"));
    MPUTR(numfields, "skipped", STRNC("skipped"));

    songlist_set_fields(fields);
    songlist_set_numeric_fields(numfields);
    songlist_set_filter(NULL);
    songlist_set_sorting(config_get("sorting"));

    REL(fields);
    REL(numfields);

    /* load database */

    char* libpath = config_get("lib_path");

    mt_time(NULL);
    lib_init();        // destroy 1
    lib_read(libpath); // read up database if exist
    mt_time("parsing database");

    /* load library */

    mt_map_t* files = MNEW(); // REL 0

    mt_time(NULL);
    fm_read_files(config_get("lib_path"), files); // read all files under library path
    mt_time("parsing library");

    if (lib_count() == 0)
    {
	// add unanalyzed files to db to show something
	mt_vector_t* songlist = VNEW();
	mt_map_values(files, songlist);
	lib_add_entries(songlist);
	// analyze all
	vmp.analyzer = analyzer_run(songlist);
	REL(songlist);
    }
    else
    {
	lib_remove_non_existing(files);
	lib_filter_existing(files);
	// analyze remaining
	mt_vector_t* remaining = VNEW();
	mt_map_values(files, remaining);
	vmp.analyzer = analyzer_run(remaining);
	REL(remaining);
    }

    REL(files);

    ui_update_songlist();
}

/* window update */

void update(ku_event_t ev)
{
    /* printf("UPDATE %i %u %i %i\n", ev.type, ev.time, ev.w, ev.h); */

    if (ev.type == KU_EVENT_FRAME)
    {
	/* check for remote commands */

	if (vmp.remote.command > 0)
	{
	    if (vmp.remote.command == 1) ui_toggle_pause();
	    if (vmp.remote.command == 2) ui_play_next();
	    if (vmp.remote.command == 3) ui_play_prev();
	    vmp.remote.command = 0;
	}

	/* check analyzer */

	if (vmp.analyzer)
	{
	    if (vmp.analyzer_ratio < vmp.analyzer->ratio)
	    {
		char text[100];
		snprintf(text, 100, "Analyzing songs %.2i%%", (int) (vmp.analyzer->ratio * 100.0));
		ui_show_progress(text);
	    }

	    if (vmp.analyzer->ratio == 1.0)
	    {
		lib_add_entries(vmp.analyzer->songs);
		if (config_get_bool("lib_organize")) lib_organize(config_get("lib_path"), lib_get_db());
		lib_write(config_get("lib_path"));
		ui_update_songlist();

		REL(vmp.analyzer);
		vmp.analyzer       = NULL;
		vmp.analyzer_ratio = 0.0;
	    }
	}

	ui_update_player();
    }

    ku_window_event(vmp.kuwindow, ev);

    if (vmp.wlwindow->frame_cb == NULL)
    {
	ku_rect_t dirty = ku_window_update(vmp.kuwindow, 0);

	if (dirty.w > 0 && dirty.h > 0)
	{
	    ku_rect_t sum = ku_rect_add(dirty, vmp.dirtyrect);

	    /* mt_log_debug("drt %i %i %i %i", (int) dirty.x, (int) dirty.y, (int) dirty.w, (int) dirty.h); */
	    /* mt_log_debug("drt prev %i %i %i %i", (int) vmp.dirtyrect.x, (int) vmp.dirtyrect.y, (int) vmp.dirtyrect.w, (int) vmp.dirtyrect.h); */
	    /* mt_log_debug("sum aftr %i %i %i %i", (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h); */

	    /* mt_time(NULL); */
	    if (vmp.softrender) ku_renderer_software_render(vmp.kuwindow->views, &vmp.wlwindow->bitmap, sum);
	    else ku_renderer_egl_render(vmp.kuwindow->views, &vmp.wlwindow->bitmap, sum);
	    /* mt_time("Render"); */
	    /* nanosleep((const struct timespec[]){{0, 100000000L}}, NULL); */

	    ku_wayland_draw_window(vmp.wlwindow, (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h);

	    vmp.dirtyrect = dirty;
	}
    }
}

void update_session(ku_event_t ev)
{
    if (ev.type == KU_EVENT_FRAME) ui_update_player();

    ku_window_event(vmp.kuwindow, ev);
    ku_window_update(vmp.kuwindow, 0);

    if (vmp.softrender) ku_renderer_software_render(vmp.kuwindow->views, &vmp.wlwindow->bitmap, vmp.kuwindow->root->frame.local);
    else ku_renderer_egl_render(vmp.kuwindow->views, &vmp.wlwindow->bitmap, vmp.kuwindow->root->frame.local);
}

/* save window buffer to png */

void update_screenshot(uint32_t frame)
{
    static int shotindex = 0;

    char* name = mt_string_new_format(20, "screenshot%.3i.png", shotindex++); // REL 1
    char* path = "";

    if (vmp.record) path = mt_path_new_append(vmp.rec_path, name); // REL 2
    if (vmp.replay) path = mt_path_new_append(vmp.rep_path, name); // REL 2

    if (vmp.softrender)
    {
	coder_write_png(path, &vmp.wlwindow->bitmap);
    }
    else
    {
	ku_bitmap_t* bitmap = ku_bitmap_new(vmp.wlwindow->width, vmp.wlwindow->height);
	ku_gl_save_framebuffer(bitmap);
	ku_bitmap_t* flipped = bm_new_flip_y(bitmap); // REL 3
	coder_write_png(path, flipped);
	REL(flipped);
	REL(bitmap);
    }
    ui_update_cursor((ku_rect_t){0, 0, vmp.wlwindow->width, vmp.wlwindow->height});

    printf("SCREENHSOT AT %u : %s\n", frame, path);
}

/* window update during recording */

void update_record(ku_event_t ev)
{
    /* normalize floats for deterministic movements during record/replay */
    ev.dx         = floor(ev.dx * 10000) / 10000;
    ev.dy         = floor(ev.dy * 10000) / 10000;
    ev.ratio      = floor(ev.ratio * 10000) / 10000;
    ev.time_frame = floor(ev.time_frame * 10000) / 10000;

    if (ev.type == KU_EVENT_FRAME)
    {
	/* record and send waiting events */
	for (int index = 0; index < vmp.eventqueue->length; index++)
	{
	    ku_event_t* event = (ku_event_t*) vmp.eventqueue->data[index];
	    event->frame      = ev.frame;
	    evrec_record(*event);

	    update_session(*event);

	    if (event->type == KU_EVENT_KDOWN && event->keycode == XKB_KEY_Print) update_screenshot(ev.frame);
	    else ui_update_cursor((ku_rect_t){event->x, event->y, 10, 10});
	}

	mt_vector_reset(vmp.eventqueue);

	/* send frame event */
	update_session(ev);

	/* force frame request if needed */
	if (vmp.wlwindow->frame_cb == NULL)
	{
	    ku_wayland_draw_window(vmp.wlwindow, 0, 0, vmp.wlwindow->width, vmp.wlwindow->height);
	}
	else mt_log_error("FRAME CALLBACK NOT NULL!!");
    }
    else
    {
	/* queue event */
	void* event = HEAP(ev);
	VADD(vmp.eventqueue, event);
    }
}

/* window update during replay */

void update_replay(ku_event_t ev)
{
    if (ev.type == KU_EVENT_FRAME)
    {
	// get recorded events
	ku_event_t* recev = NULL;
	while ((recev = evrec_replay(ev.frame)) != NULL)
	{
	    update_session(*recev);

	    if (recev->type == KU_EVENT_KDOWN && recev->keycode == XKB_KEY_Print) update_screenshot(ev.frame);
	    else ui_update_cursor((ku_rect_t){recev->x, recev->y, 10, 10});
	}

	/* send frame event */
	update_session(ev);

	/* force frame request if needed */
	if (vmp.wlwindow->frame_cb == NULL) ku_wayland_draw_window(vmp.wlwindow, 0, 0, vmp.wlwindow->width, vmp.wlwindow->height);
	else mt_log_error("FRAME CALLBACK NOT NULL!!");
    }
}

void destroy()
{
    ku_wayland_delete_window(vmp.wlwindow);

    if (vmp.replay) evrec_destroy();
    if (vmp.record) evrec_destroy();

    ui_destroy();

    REL(vmp.kuwindow);

    if (!vmp.softrender) ku_renderer_egl_destroy();

    SDL_Quit();

    if (vmp.rec_path) REL(vmp.rec_path); // REL 14
    if (vmp.rep_path) REL(vmp.rep_path); // REL 15
}

int main(int argc, char* argv[])
{
    mt_log_use_colors(isatty(STDERR_FILENO));
    mt_log_level_info();
    mt_time(NULL);

    printf("Visual Music Player v" VMP_VERSION
	   " by Milan Toth ( www.milgra.com )\n"
	   "If you like this app try :\n"
	   "- Sway Oveview ( github.com/milgra/sov )\n"
	   "- Wayland Control Panel (github.com/milgra/wcp)\n"
	   "- Multimedia File Manager (github.com/milgra/vmp)\n"
	   "- SwayOS (swayos.github.io)\n"
	   "Games :\n"
	   "- Brawl (github.com/milgra/brawl)\n"
	   "- Cortex ( github.com/milgra/cortex )\n"
	   "- Termite (github.com/milgra/termite)\n\n");

    const char* usage =
	"Usage: vmp [options]\n"
	"\n"
	"  -h, --help                            Show help message and quit.\n"
	"  -v, --verbose                         Increase verbosity of messages, defaults to errors and warnings only.\n"
	"  -l --library= [library path]          Library path, ~/Music by default\n"
	"  -o --organize                         Organize library, rename new songs to /artist/album/trackno title.ext\n"
	/* "  -c --config= [config file]            Use config file for session\n" */
	"  -r --resources= [resources folder]    Resources dir for current session\n"
	"  -s --record= [recorder file]          Record session to file\n"
	"  -p --replay= [recorder file]          Replay session from file\n"
	"  -f --frame= [widthxheight]            Initial window dimension\n"
	"\n";

    const struct option long_options[] =
	{
	    {"help", no_argument, NULL, 'h'},
	    {"verbose", no_argument, NULL, 'v'},
	    {"library", optional_argument, 0, 'l'},
	    {"organize", optional_argument, 0, 'l'},
	    {"resources", optional_argument, 0, 'r'},
	    {"record", optional_argument, 0, 's'},
	    {"replay", optional_argument, 0, 'p'},
	    {"config", optional_argument, 0, 'c'},
	    {"frame", optional_argument, 0, 'f'}};

    char* cfg_par = NULL;
    char* res_par = NULL;
    char* rec_par = NULL;
    char* rep_par = NULL;
    char* frm_par = NULL;
    char* org_par = "false";
    char* lib_par = NULL;

    int verbose      = 0;
    int option       = 0;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "vhr:s:p:c:f:l:o", long_options, &option_index)) != -1)
    {
	switch (option)
	{
	    case '?': printf("parsing option %c value: %s\n", option, optarg); break;
	    case 'c': cfg_par = STRNC(optarg); break; // REL 0
	    case 'l': lib_par = STRNC(optarg); break; // REL 1
	    case 'o': org_par = "true"; break;
	    case 'r': res_par = STRNC(optarg); break; // REL 1
	    case 's': rec_par = STRNC(optarg); break; // REL 2
	    case 'p': rep_par = STRNC(optarg); break; // REL 3
	    case 'f': frm_par = STRNC(optarg); break; // REL 4
	    case 'v': verbose = 1; break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    if (rec_par) vmp.record = 1;
    if (rep_par) vmp.replay = 1;

    srand((unsigned int) time(NULL));

    char cwd[PATH_MAX] = {"~"};
    getcwd(cwd, sizeof(cwd));

    char* wrk_path    = mt_path_new_normalize(cwd, NULL); // REL 5
    char* lib_path    = lib_par ? mt_path_new_normalize(lib_par, wrk_path) : mt_path_new_normalize("~/Music", wrk_path);
    char* res_path    = res_par ? mt_path_new_normalize(res_par, wrk_path) : STRNC(PKG_DATADIR);                                     // REL 7
    char* cfgdir_path = cfg_par ? mt_path_new_normalize(cfg_par, wrk_path) : mt_path_new_normalize("~/.config/vmp", getenv("HOME")); // REL 8
    char* css_path    = mt_path_new_append(res_path, "html/main.css");                                                               // REL 9
    char* html_path   = mt_path_new_append(res_path, "html/main.html");                                                              // REL 10
    char* cfg_path    = mt_path_new_append(cfgdir_path, "config.kvl");                                                               // REL 12
    char* per_path    = mt_path_new_append(cfgdir_path, "state.kvl");                                                                // REL 13
    char* rec_path    = rec_par ? mt_path_new_normalize(rec_par, wrk_path) : NULL;                                                   // REL 14
    char* rep_path    = rep_par ? mt_path_new_normalize(rep_par, wrk_path) : NULL;                                                   // REL 15

    // print path info to console

    printf("library path  : %s\n", lib_path);
    printf("working path  : %s\n", wrk_path);
    printf("resource path : %s\n", res_path);
    printf("config path   : %s\n", cfg_path);
    printf("state path    : %s\n", per_path);
    printf("css path      : %s\n", css_path);
    printf("html path     : %s\n", html_path);
    printf("record path   : %s\n", rec_path);
    printf("replay path   : %s\n", rep_path);
    printf("\n");

    if (verbose) mt_log_inc_verbosity();

    // init config

    config_init(); // DESTROY 0

    // set default values

    config_set("fields", "artist 200 album 200 title 350 date 60 genre 100 track 60 disc 60 duration 50 channels 40 bitrate 100 samplerate 80 plays 55 skips 55 added 150 type 80 container 80");
    config_set("sorting", "artist 1 album 1 track 1");

    // read config, it overwrites defaults if exists

    config_read(cfg_path);

    // init non-configurable defaults

    config_set("cfg_path", cfg_path);
    config_set("res_path", res_path);
    config_set("lib_path", lib_path);
    config_set("lib_organize", org_par);
    config_set("css_path", css_path);
    config_set("html_path", html_path);

    if (rec_path) config_set("rec_path", rec_path);
    if (rep_path) config_set("rep_path", rep_path);

    mt_time("config parsing");

    /* this two shouldn't go into the config file because of record/replay */

    vmp.rec_path = rec_path;
    vmp.rep_path = rep_path;

    if (rec_path) evrec_init_recorder(rec_path); // DESTROY 4
    if (rep_path) evrec_init_player(rep_path);

    if (rec_path != NULL) ku_wayland_init(init, update_record, destroy, 0);
    else if (rep_path != NULL) ku_wayland_init(init, update_replay, destroy, 16);
    else ku_wayland_init(init, update, destroy, 0);

    config_destroy(); // DESTROY 0

    // cleanup

    if (cfg_par) REL(cfg_par); // REL 0
    if (lib_par) REL(lib_par); // REL 1
    if (res_par) REL(res_par); // REL 1
    if (rec_par) REL(rec_par); // REL 2
    if (rep_par) REL(rep_par); // REL 3
    if (frm_par) REL(frm_par); // REL 4

    REL(wrk_path);    // REL 6
    REL(lib_path);    // REL 7
    REL(res_path);    // REL 7
    REL(cfgdir_path); // REL 8
    REL(css_path);    // REL 9
    REL(html_path);   // REL 10
    REL(cfg_path);    // REL 12
    REL(per_path);    // REL 13

    if (rec_path) REL(rec_path); // REL 14
    if (rep_path) REL(rep_path); // REL 15

#ifdef MT_MEMORY_DEBUG
    mt_memory_stats();
#endif

    return 0;
}
