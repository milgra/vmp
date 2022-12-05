#include "analyzer.c"
#include "coder.c"
#include "config.c"
#include "evrecorder.c"
#include "filemanager.c"
#include "ku_bitmap_ext.c"
#include "ku_connector_wayland.c"
#include "ku_gl.c"
#include "ku_recorder.c"
#include "ku_renderer_egl.c"
#include "ku_renderer_soft.c"
#include "ku_window.c"
#include "library.c"
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
    wl_window_t* wlwindow;
    ku_window_t* kuwindow;

    ku_rect_t dirtyrect;
    int       softrender;

    float       analyzer_ratio;
    analyzer_t* analyzer;

    int      frames;
    remote_t remote;

    int   autotest;
    char* pngpath;

    int width;
    int height;
} vmp = {0};

void init(wl_event_t event)
{
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_AUDIO);

    if (vmp.softrender)
    {
	vmp.wlwindow = ku_wayland_create_window("vmp", vmp.width, vmp.height);
	ku_wayland_show_window(vmp.wlwindow);
    }
    else
    {
	vmp.wlwindow = ku_wayland_create_eglwindow("vmp", vmp.width, vmp.height);
	ku_wayland_show_window(vmp.wlwindow);

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
}

void load(wl_window_t* info)
{
    vmp.kuwindow = ku_window_create(info->buffer_width, info->buffer_height, info->scale);

    ui_init(info->buffer_width, info->buffer_height, info->scale, vmp.kuwindow); // DESTROY 3

    if (vmp.autotest) ui_add_cursor();

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

    mt_vector_t* entries = VNEW();
    lib_get_entries(entries);
    songlist_set_songs(entries);
    REL(entries);

    ui_update_songlist();

    ku_window_layout(vmp.kuwindow);
}

/* window update */

void update(ku_event_t ev)
{
    /* printf("UPDATE %i %u %i %i\n", ev.type, ev.time, ev.w, ev.h); */

    if (ev.type == KU_EVENT_WINDOW_SHOWN) load(ev.window);

    if (ev.type == KU_EVENT_FRAME || ev.type == KU_EVENT_TIME)
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

    if (vmp.autotest)
    {
	/* create screenshot if needed */
	if (ev.type == KU_EVENT_KEY_DOWN && ev.keycode == XKB_KEY_Print)
	{
	    static int shotindex = 0;

	    char* name = mt_string_new_format(20, "screenshot%.3i.png", shotindex++);
	    char* path = mt_path_new_append(vmp.pngpath, name);

	    if (vmp.softrender) ku_renderer_soft_screenshot(&vmp.wlwindow->bitmap, path);
	    else ku_renderer_egl_screenshot(&vmp.wlwindow->bitmap, path);

	    ui_update_cursor((ku_rect_t){0, 0, vmp.wlwindow->width, vmp.wlwindow->height});

	    printf("SCREENHSOT AT %u : %s\n", ev.frame, path);
	}
	else if (ev.x > 0 && ev.y > 0) ui_update_cursor((ku_rect_t){ev.x, ev.y, 10, 10}); /* update virtual cursor if needed */
    }

    if (vmp.wlwindow->frame_cb == NULL)
    {
	ku_rect_t dirty = ku_window_update(vmp.kuwindow, 0);

	/* in case of record/replay force continuous draw by full size dirty rect */
	if (vmp.autotest) dirty = vmp.kuwindow->root->frame.local;

	if (dirty.w > 0 && dirty.h > 0)
	{
	    ku_rect_t sum = ku_rect_add(dirty, vmp.dirtyrect);

	    if (vmp.softrender) ku_renderer_software_render(vmp.kuwindow->views, &vmp.wlwindow->bitmap, sum);
	    else ku_renderer_egl_render(vmp.kuwindow->views, &vmp.wlwindow->bitmap, sum);

	    /* frame done request must be requested before draw */
	    ku_wayland_request_frame(vmp.wlwindow);
	    ku_wayland_draw_window(vmp.wlwindow, (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h);

	    /* store current dirty rect for next draw */
	    vmp.dirtyrect = dirty;
	}
    }
}

void destroy()
{
    ui_destroy();
    REL(vmp.kuwindow);
    ku_wayland_delete_window(vmp.wlwindow);

    lib_destroy();

    if (!vmp.softrender) ku_renderer_egl_destroy();

    SDL_Quit();
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
	    {"software_renderer", optional_argument, 0, 0},
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
	    case 0:
		if (option_index == 5) vmp.softrender = 1;
		/* printf("option %i %s", option_index, long_options[option_index].name); */
		/* if (optarg) printf(" with arg %s", optarg); */
		break;
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

    srand((unsigned int) time(NULL));

    char cwd[PATH_MAX] = {"~"};
    getcwd(cwd, sizeof(cwd));

    char* wrk_path    = mt_path_new_normalize(cwd, NULL); // REL 5
    char* lib_path    = lib_par ? mt_path_new_normalize(lib_par, wrk_path) : mt_path_new_normalize("~/Music", wrk_path);
    char* res_path    = res_par ? mt_path_new_normalize(res_par, wrk_path) : STRNC(PKG_DATADIR);                                     // REL 7
    char* cfgdir_path = cfg_par ? mt_path_new_normalize(cfg_par, wrk_path) : mt_path_new_normalize("~/.config/vmp", getenv("HOME")); // REL 8
    char* img_path    = mt_path_new_append(res_path, "img");                                                                         // REL 9
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
    printf("img path      : %s\n", img_path);
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

    config_set("img_path", img_path);
    config_set("cfg_path", cfg_path);
    config_set("res_path", res_path);
    config_set("lib_path", lib_path);
    config_set("lib_organize", org_par);
    config_set("css_path", css_path);
    config_set("html_path", html_path);

    if (frm_par != NULL)
    {
	vmp.width  = atoi(frm_par);
	char* next = strstr(frm_par, "x");
	vmp.height = atoi(next + 1);
    }
    else
    {
	/* TODO calc this based on output size */
	vmp.width  = 1200;
	vmp.height = 600;
    }

    if (rec_path || rep_path) ku_recorder_init(update);
    if (rec_path || rep_path) vmp.autotest = 1;
    if (rec_path)
    {
	char* tgt_path = mt_path_new_append(rec_path, "session.rec");
	ku_recorder_record(tgt_path);
	REL(tgt_path);
	vmp.pngpath = rec_path;
    }
    if (rep_path)
    {
	char* tgt_path = mt_path_new_append(rep_path, "session.rec");
	ku_recorder_replay(tgt_path);
	REL(tgt_path);
	vmp.pngpath = rep_path;
    }

    /* proxy events through the recorder in case of record/replay */
    if (rec_path || rep_path) ku_wayland_init(init, ku_recorder_update, destroy, 0);
    else ku_wayland_init(init, update, destroy, 1000); // needs second updates because of time field

    config_destroy(); // DESTROY 0

    // cleanup

    ku_recorder_destroy();

    if (cfg_par) REL(cfg_par); // REL 0
    if (lib_par) REL(lib_par); // REL 1
    if (res_par) REL(res_par); // REL 1
    if (rec_par) REL(rec_par); // REL 2
    if (rep_par) REL(rep_par); // REL 3
    if (frm_par) REL(frm_par); // REL 4

    REL(img_path);
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
