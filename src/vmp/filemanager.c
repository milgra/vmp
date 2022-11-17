#ifndef filemanager_h
#define filemanager_h

#include "mt_channel.c"
#include "mt_map.c"

void fm_read_files(char* libpath, mt_map_t* db);
int  fm_create(char* file_path, mode_t mode);
void fm_delete_file(char* libpath, mt_map_t* entry);
int  fm_rename_file(char* old, char* new, char* new_dirs);

#endif

#if __INCLUDE_LEVEL__ == 0

#define __USE_XOPEN_EXTENDED 1 // needed for linux
#include <ftw.h>

#include "coder.c"
#include "filemanager.c"
#include "mt_log.c"
#include "mt_path.c"
#include "mt_string.c"
#include <SDL.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>

static int fm_file_data_step(const char* fpath, const struct stat* sb, int tflag, struct FTW* ftwbuf);
int        analyzer_thread(void* chptr);

struct fm_t
{
    mt_map_t*    files;
    mt_vector_t* paths;
    char         lock;
    char*        path;
} lib = {0};

void fm_read_files(char* fm_path, mt_map_t* files)
{
    assert(fm_path != NULL);

    lib.files = files;
    lib.path  = fm_path;

    nftw(fm_path, fm_file_data_step, 20, FTW_PHYS);

    mt_log_debug("lib : scanned, files : %i", files->count);
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
	mt_map_t* song = MNEW();

	char* path = STRNC((char*) fpath + strlen(lib.path) + 1);
	char* size = STRNF(20, "%li", sb->st_size); // REL 0

	// add file data

	MPUTR(song, "path", path);
	MPUTR(song, "size", size);
	MPUTR(song, "added", STRNC("..."));
	MPUTR(song, "played", STRNC("..."));
	MPUTR(song, "skipped", STRNC("..."));
	MPUTR(song, "plays", STRNC("..."));
	MPUTR(song, "skips", STRNC("..."));
	MPUTR(song, "artist", STRNC("..."));
	MPUTR(song, "album", STRNC("..."));
	MPUTR(song, "title", mt_path_new_filename(path));

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

void fm_delete_file(char* fm_path, mt_map_t* entry)
{
    assert(fm_path != NULL);

    char* rel_path  = MGET(entry, "path");
    char* file_path = STRNF(PATH_MAX + NAME_MAX, "%s/%s", fm_path, rel_path); // REL 0

    int error = remove(file_path);
    if (error)
	mt_log_debug("lib : cannot remove file %s : %s", file_path, strerror(errno));
    else
	mt_log_debug("lib : file %s removed.", file_path);

    REL(file_path); // REL 0
}

int fm_rename_file(char* old_path, char* new_path, char* new_dirs)
{
    mt_log_debug("lib : renaming %s to %s", old_path, new_path);

    int error = fm_create(new_dirs, 0777);

    if (error == 0)
    {
	error = rename(old_path, new_path);
	return error;
    }
    return error;
}

#endif
