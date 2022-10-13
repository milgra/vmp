#include "analyzer.c"
#include "config.c"
#include "evrecorder.c"
#include "filemanager.c"
#include "library.c"
#include "remote.c"
#include "songlist.c"
#include "ui.c"
#include "ui_compositor.c"
#include "ui_manager.c"
#include "wm_connector.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_map.c"
#include "zc_path.c"
#include "zc_time.c"
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct
{
    char replay;
    char record;

    float       analyzer_ratio;
    analyzer_t* analyzer;

    int      frames;
    remote_t remote;
} vmp = {0};

void init(int width, int height)
{
    zc_time(NULL);
    ui_init(width, height); // DESTROY 3
    zc_time("ui init");

    if (vmp.record)
    {
	evrec_init_recorder(config_get("rec_path")); // DESTROY 4
    }

    if (vmp.replay)
    {
	evrec_init_player(config_get("rep_path")); // DESTROY 5
	ui_add_cursor();
    }

    remote_listen(&vmp.remote);
}

void post_render_init()
{
    if (vmp.frames < 3) vmp.frames += 1;
    else if (vmp.frames == 3)
    {
	vmp.frames = 4;

	map_t* fields = MNEW();

	MPUTR(fields, "artist", cstr_new_cstring("artist"));
	MPUTR(fields, "album", cstr_new_cstring("album"));
	MPUTR(fields, "title", cstr_new_cstring("title"));
	MPUTR(fields, "date", cstr_new_cstring("date"));
	MPUTR(fields, "genre", cstr_new_cstring("genre"));
	MPUTR(fields, "track", cstr_new_cstring("track"));
	MPUTR(fields, "disc", cstr_new_cstring("disc"));
	MPUTR(fields, "duration", cstr_new_cstring("duration"));
	MPUTR(fields, "channels", cstr_new_cstring("channels"));
	MPUTR(fields, "bitrate", cstr_new_cstring("bitrate"));
	MPUTR(fields, "samplerate", cstr_new_cstring("samplerate"));
	MPUTR(fields, "plays", cstr_new_cstring("plays"));
	MPUTR(fields, "skips", cstr_new_cstring("skips"));
	MPUTR(fields, "added", cstr_new_cstring("added"));
	MPUTR(fields, "played", cstr_new_cstring("played"));
	MPUTR(fields, "skipped", cstr_new_cstring("skipped"));
	MPUTR(fields, "type", cstr_new_cstring("type"));
	MPUTR(fields, "container", cstr_new_cstring("container"));

	map_t* numfields = MNEW();

	MPUTR(numfields, "track", cstr_new_cstring("track"));
	MPUTR(numfields, "disc", cstr_new_cstring("disc"));
	MPUTR(numfields, "duration", cstr_new_cstring("duration"));
	MPUTR(numfields, "channels", cstr_new_cstring("channels"));
	MPUTR(numfields, "bitrate", cstr_new_cstring("bitrate"));
	MPUTR(numfields, "samplerate", cstr_new_cstring("samplerate"));
	MPUTR(numfields, "plays", cstr_new_cstring("plays"));
	MPUTR(numfields, "skips", cstr_new_cstring("skips"));
	MPUTR(numfields, "added", cstr_new_cstring("added"));
	MPUTR(numfields, "played", cstr_new_cstring("played"));
	MPUTR(numfields, "skipped", cstr_new_cstring("skipped"));

	songlist_set_fields(fields);
	songlist_set_numeric_fields(numfields);
	songlist_set_filter(NULL);
	songlist_set_sorting("artist 1 album 1 track 1");

	REL(fields);
	REL(numfields);

	/* load database */

	char* libpath = config_get("lib_path");

	zc_time(NULL);
	lib_init();        // destroy 1
	lib_read(libpath); // read up database if exist
	zc_time("parsing database");

	/* load library */

	map_t* files = MNEW(); // REL 0

	zc_time(NULL);
	fm_read_files(config_get("lib_path"), files); // read all files under library path
	zc_time("parsing library");

	if (lib_count() == 0)
	{
	    // add unanalyzed files to db to show something
	    vec_t* songlist = VNEW();
	    map_values(files, songlist);
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
	    vec_t* remaining = VNEW();
	    map_values(files, remaining);
	    vmp.analyzer = analyzer_run(remaining);
	    REL(remaining);
	}

	REL(files);

	ui_update_songlist();
    }
}

void update(ev_t ev)
{
    if (ev.type == EV_TIME)
    {
	/* check init */

	if (vmp.frames < 4) post_render_init();

	/* check remote */

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

	if (vmp.replay)
	{
	    // get recorded events
	    ev_t* recev = NULL;
	    while ((recev = evrec_replay(ev.time)) != NULL)
	    {
		ui_manager_event(*recev);
		ui_update_cursor((r2_t){recev->x, recev->y, 10, 10});

		if (recev->type == EV_KDOWN && recev->keycode == SDLK_PRINTSCREEN) ui_screenshot(ev.time);
	    }
	}
    }
    else
    {
	if (vmp.record)
	{
	    evrec_record(ev);
	    if (ev.type == EV_KDOWN && ev.keycode == SDLK_PRINTSCREEN) ui_screenshot(ev.time);
	}
    }

    // in case of replay only send time events
    if (!vmp.replay || ev.type == EV_TIME) ui_manager_event(ev);
}

void render(uint32_t time)
{
    ui_manager_render(time);
}

void destroy()
{
    if (vmp.replay) evrec_destroy(); // DESTROY 5
    if (vmp.record) evrec_destroy(); // DESTROY 4

    ui_destroy(); // DESTROY 3
}

int main(int argc, char* argv[])
{
    zc_log_use_colors(isatty(STDERR_FILENO));
    zc_log_level_info();
    zc_time(NULL);

    printf("Visual Music Player v" VMP_VERSION " by Milan Toth ( www.milgra.com )\n");
    printf("If you like this app try Multimedia File Manager (github.com/milgra/vmp) or Sway Oveview ( github.com/milgra/sov )\n");
    printf("Or my games : Cortex ( github.com/milgra/cortex ), Termite (github.com/milgra/termite) or Brawl (github.com/milgra/brawl)\n\n");

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
	    case 'c': cfg_par = cstr_new_cstring(optarg); break; // REL 0
	    case 'l': lib_par = cstr_new_cstring(optarg); break; // REL 1
	    case 'o': org_par = "true"; break;
	    case 'r': res_par = cstr_new_cstring(optarg); break; // REL 1
	    case 's': rec_par = cstr_new_cstring(optarg); break; // REL 2
	    case 'p': rep_par = cstr_new_cstring(optarg); break; // REL 3
	    case 'f': frm_par = cstr_new_cstring(optarg); break; // REL 4
	    case 'v': verbose = 1; break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    if (rec_par) vmp.record = 1;
    if (rep_par) vmp.replay = 1;

    srand((unsigned int) time(NULL));

    char cwd[PATH_MAX] = {"~"};
    getcwd(cwd, sizeof(cwd));

    char* top_path = path_new_normalize(cwd, NULL); // REL 5
    char* sdl_base = SDL_GetBasePath();
    char* wrk_path = path_new_normalize(sdl_base, NULL); // REL 6
    SDL_free(sdl_base);
    char* lib_path    = lib_par ? path_new_normalize(lib_par, wrk_path) : path_new_normalize("~/Music", wrk_path);
    char* res_path    = res_par ? path_new_normalize(res_par, wrk_path) : cstr_new_cstring(PKG_DATADIR);                       // REL 7
    char* cfgdir_path = cfg_par ? path_new_normalize(cfg_par, wrk_path) : path_new_normalize("~/.config/vmp", getenv("HOME")); // REL 8
    char* css_path    = path_new_append(res_path, "html/main.css");                                                            // REL 9
    char* html_path   = path_new_append(res_path, "html/main.html");                                                           // REL 10
    char* cfg_path    = path_new_append(cfgdir_path, "config.kvl");                                                            // REL 12
    char* per_path    = path_new_append(cfgdir_path, "state.kvl");                                                             // REL 13
    char* rec_path    = rec_par ? path_new_normalize(rec_par, wrk_path) : NULL;                                                // REL 14
    char* rep_path    = rep_par ? path_new_normalize(rep_par, wrk_path) : NULL;                                                // REL 15

    // print path info to console

    printf("library path  : %s\n", lib_path);
    printf("top path      : %s\n", top_path);
    printf("working path  : %s\n", wrk_path);
    printf("resource path : %s\n", res_path);
    printf("config path   : %s\n", cfg_path);
    printf("state path    : %s\n", per_path);
    printf("css path      : %s\n", css_path);
    printf("html path     : %s\n", html_path);
    printf("record path   : %s\n", rec_path);
    printf("replay path   : %s\n", rep_path);
    printf("\n");

    if (verbose) zc_log_inc_verbosity();

    // init config

    config_init(); // DESTROY 0

    config_set("res_path", res_path);
    config_set("lib_path", lib_path);
    config_set("lib_organize", org_par);

    // read config, it overwrites defaults if exists

    config_read(cfg_path);

    // init non-configurable defaults

    config_set("top_path", top_path);
    config_set("wrk_path", wrk_path);
    config_set("cfg_path", cfg_path);
    config_set("per_path", per_path);
    config_set("css_path", css_path);
    config_set("html_path", html_path);

    if (rec_path) config_set("rec_path", rec_path);
    if (rep_path) config_set("rep_path", rep_path);

    zc_time("config parsing");

    wm_loop(init, update, render, destroy, frm_par);

    config_destroy(); // DESTROY 0

    // cleanup

    if (cfg_par) REL(cfg_par); // REL 0
    if (lib_par) REL(lib_par); // REL 1
    if (res_par) REL(res_par); // REL 1
    if (rec_par) REL(rec_par); // REL 2
    if (rep_par) REL(rep_par); // REL 3
    if (frm_par) REL(frm_par); // REL 4

    REL(top_path);    // REL 5
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

#ifdef DEBUG
	/* mem_stats(); */
#endif

    return 0;
}
