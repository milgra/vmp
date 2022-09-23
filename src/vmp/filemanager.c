#ifndef filemanager_h
#define filemanager_h

#include "zc_channel.c"
#include "zc_map.c"

void fm_read_files(char* libpath, map_t* db);
int  fm_create(char* file_path, mode_t mode);
void fm_delete_file(char* libpath, map_t* entry);
int  fm_rename_file(char* old, char* new, char* new_dirs);

#endif

#if __INCLUDE_LEVEL__ == 0

#define __USE_XOPEN_EXTENDED 1 // needed for linux
#include <ftw.h>

#include "coder.c"
#include "filemanager.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_path.c"
#include <SDL.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>

static int fm_file_data_step(const char* fpath, const struct stat* sb, int tflag, struct FTW* ftwbuf);
int        analyzer_thread(void* chptr);

struct fm_t
{
    map_t* files;
    vec_t* paths;
    char   lock;
    char*  path;
} lib = {0};

void fm_read_files(char* fm_path, map_t* files)
{
    assert(fm_path != NULL);

    lib.files = files;
    lib.path  = fm_path;

    nftw(fm_path, fm_file_data_step, 20, FTW_PHYS);

    zc_log_debug("lib : scanned, files : %i", files->count);
}

static int fm_file_data_step(const char* fpath, const struct stat* sb, int tflag, struct FTW* ftwbuf)
{
    /* printf("%-3s %2d %7jd   %-40s %d %s\n", */
    /*        (tflag == FTW_D) ? "d" : (tflag == FTW_DNR) ? "dnr" : (tflag == FTW_DP) ? "dp" : (tflag == FTW_F) ? "f" : (tflag == FTW_NS) ? "ns" : (tflag == FTW_SL) ? "sl" : (tflag == FTW_SLN) ? "sln" : "???", */
    /*        ftwbuf->level, */
    /*        (intmax_t)sb->st_size, */
    /*        fpath, */
    /*        ftwbuf->base, */
    /*        fpath + ftwbuf->base); */

    if (tflag == FTW_F)
    {
	map_t* song = MNEW();

	char* path = cstr_new_cstring((char*) fpath + strlen(lib.path) + 1);
	char* size = cstr_new_format(20, "%li", sb->st_size); // REL 0

	// add file data

	MPUTR(song, "path", path);
	MPUTR(song, "size", size);
	MPUTR(song, "added", cstr_new_cstring("..."));
	MPUTR(song, "played", cstr_new_cstring("..."));
	MPUTR(song, "skipped", cstr_new_cstring("..."));
	MPUTR(song, "plays", cstr_new_cstring("..."));
	MPUTR(song, "skips", cstr_new_cstring("..."));
	MPUTR(song, "artist", cstr_new_cstring("..."));
	MPUTR(song, "album", cstr_new_cstring("..."));
	MPUTR(song, "title", path_new_filename(path));

	MPUTR(lib.files, fpath + strlen(lib.path) + 1, song); // use relative path as path

	/* char* size = cstr_new_format(20, "%li", sb->st_size); // REL 0 */
	/* MPUT(lib.files, fpath + strlen(lib.path) + 1, size);  // use relative path as path */
	/* REL(size);                                            // REL 0 */
    }

    return 0; /* To tell nftw() to continue */
}

int fm_create(char* file_path, mode_t mode)
{
    assert(file_path && *file_path);
    for (char* p = strchr(file_path + 1, '/');
	 p;
	 p = strchr(p + 1, '/'))
    {
	*p = '\0';
	if (mkdir(file_path, mode) == -1)
	{
	    if (errno != EEXIST)
	    {
		*p = '/';
		return -1;
	    }
	}
	*p = '/';
    }
    return 0;
}

void fm_delete_file(char* fm_path, map_t* entry)
{
    assert(fm_path != NULL);

    char* rel_path  = MGET(entry, "file/path");
    char* file_path = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s", fm_path, rel_path); // REL 0

    int error = remove(file_path);
    if (error)
	zc_log_debug("lib : cannot remove file %s : %s", file_path, strerror(errno));
    else
	zc_log_debug("lib : file %s removed.", file_path);

    REL(file_path); // REL 0
}

int fm_rename_file(char* old_path, char* new_path, char* new_dirs)
{
    zc_log_debug("lib : renaming %s to %s", old_path, new_path);

    int error = fm_create(new_dirs, 0777);

    if (error == 0)
    {
	error = rename(old_path, new_path);
	return error;
    }
    return error;
}

#endif
