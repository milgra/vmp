#ifndef lib_h
#define lib_h

#include "mt_map.c"

void lib_init();
void lib_destroy();
void lib_read(char* libpath);
void lib_write(char* libpath);
void lib_add_entries(mt_vector_t* entries);
void lib_add_entry(char* path, mt_map_t* entry);
void lib_remove_entry(mt_map_t* entry);
void lib_remove_non_existing(mt_map_t* files);
void lib_filter_existing(mt_map_t* files);
int  lib_organize_entry(char* libpath, mt_map_t* db, mt_map_t* entry);
int  lib_organize(char* libpath, mt_map_t* db);

void lib_get_genres(mt_vector_t* vec);
void lib_get_artists(mt_vector_t* vec);

mt_map_t* lib_get_db();
void      lib_get_entries(mt_vector_t* entries);
uint32_t  lib_count();
void      lib_reset();

void lib_update_metadata(char* path, mt_map_t* changed, mt_vector_t* removed);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "filemanager.c"
#include "kvlist.c"
#include "mt_log.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_vector.c"
#include <ctype.h>
#include <limits.h>

mt_map_t* db;

void lib_init()
{
    db = MNEW(); // REL 0
}

void lib_destroy()
{
    // mem_describe(db, 0);

    REL(db); // REL 1
}

void lib_read(char* libpath)
{
    assert(libpath != NULL);

    char* dbpath = mt_path_new_append(libpath, "vmp.kvl"); // REL 0

    mt_log_info("READING DB %s", dbpath);

    kvlist_read(dbpath, db, "path");

    mt_log_info("%i ENTRIES LOADED", db->count);

    REL(dbpath); // REL 0
}

void lib_write(char* libpath)
{
    assert(libpath != NULL);

    char* dbpath = STRNF(PATH_MAX + NAME_MAX, "/%s/vmp.kvl", libpath); // REL 0

    int res = kvlist_write(dbpath, db);

    if (res < 0) mt_log_debug("ERROR lib_write cannot write database %s\n", dbpath);

    mt_log_info("%i ENTRIES WRITTEN", db->count);

    REL(dbpath); // REL 0
}

void lib_add_entry(char* path, mt_map_t* entry)
{
    printf("adding entry %s\n", path);
    MPUT(db, path, entry);
}

void lib_remove_entry(mt_map_t* entry)
{
    char* path = MGET(entry, "path");
    MDEL(db, path);
}

void lib_add_entries(mt_vector_t* entries)
{
    for (int index = 0; index < entries->length; index++)
    {
	mt_map_t* entry = entries->data[index];
	char*     path  = MGET(entry, "path");
	if (path) lib_add_entry(path, entry);
    }
}

void lib_reset()
{
    mt_map_reset(db);
}

mt_map_t* lib_get_db()
{
    return db;
}

void lib_get_entries(mt_vector_t* entries)
{
    mt_map_values(db, entries);
}

uint32_t lib_count()
{
    return db->count;
}

void lib_remove_non_existing(mt_map_t* files)
{
    // first remove deleted files from db
    mt_vector_t* paths = VNEW(); // REL 0
    mt_map_keys(db, paths);

    for (int index = 0; index < paths->length; index++)
    {
	char*     path = paths->data[index];
	mt_map_t* map  = MGET(files, path);
	if (!map)
	{
	    // db path is missing from file path, file was removed
	    MDEL(db, path);
	    printf("LOG file is missing for path %s, song entry was removed from db\n", path);
	}
    }

    REL(paths);
}

void lib_filter_existing(mt_map_t* files)
{
    mt_vector_t* paths = VNEW(); // REL 0
    mt_map_keys(files, paths);

    for (int index = 0; index < paths->length; index++)
    {
	char*     path = paths->data[index];
	mt_map_t* map  = MGET(db, path);
	if (map)
	{
	    // path exist in db, removing entry from files
	    MDEL(files, path);
	}
    }

    REL(paths); // REL 0
}

char* lib_replace_char(char* str, char find, char replace)
{
    char* current_pos = strchr(str, find);
    while (current_pos)
    {
	*current_pos = replace;
	current_pos  = strchr(current_pos + 1, find);
    }
    return str;
}

int lib_organize_entry(char* libpath, mt_map_t* db, mt_map_t* entry)
{
    assert(libpath != NULL);

    int changed = 0;

    char* path   = MGET(entry, "path");
    char* artist = MGET(entry, "artist");
    char* album  = MGET(entry, "album");
    char* title  = MGET(entry, "title");
    char* track  = MGET(entry, "track");

    // remove slashes before directory creation

    lib_replace_char(artist, '/', ' ');
    lib_replace_char(title, '/', ' ');

    // get extension

    char* ext = mt_path_new_extension(path); // REL -1

    char* old_path     = STRNF(PATH_MAX + NAME_MAX, "%s/%s", libpath, path);              // REL 0
    char* new_dirs     = STRNF(PATH_MAX + NAME_MAX, "%s/%s/%s/", libpath, artist, album); // REL 1
    char* new_path     = NULL;
    char* new_path_rel = NULL;

    if (track)
    {
	int trackno  = atoi(track);
	new_path     = STRNF(PATH_MAX + NAME_MAX, "%s/%s/%s/%.3i %s.%s", libpath, artist, album, trackno, title, ext); // REL 2
	new_path_rel = STRNF(PATH_MAX + NAME_MAX, "%s/%s/%.3i %s.%s", artist, album, trackno, title, ext);             // REL 3
    }
    else
    {
	new_path     = STRNF(PATH_MAX + NAME_MAX, "%s/%s/%s/%s.%s", libpath, artist, album, title, ext); // REL 2
	new_path_rel = STRNF(PATH_MAX + NAME_MAX, "%s/%s/%s.%s", artist, album, title, ext);             // REL 3
    }

    if (strcmp(old_path, new_path) != 0)
    {
	int error = fm_rename_file(old_path, new_path, new_dirs);
	if (error == 0)
	{
	    mt_log_debug("db : updating path");
	    MPUT(entry, "path", new_path_rel);
	    MPUT(db, new_path_rel, entry);
	    MDEL(db, path);
	    changed = 1;
	}
    }

    REL(ext);          // REL -1
    REL(old_path);     // REL 0
    REL(new_dirs);     // REL 1
    REL(new_path);     // REL 2
    REL(new_path_rel); // REL 3

    return 0;
}

int lib_organize(char* libpath, mt_map_t* db)
{
    mt_log_debug("db : organizing database");

    // go through all db entries, check path, move if needed

    int          changed = 0;
    mt_vector_t* paths   = VNEW(); // REL 0

    mt_map_keys(db, paths);

    for (int index = 0; index < paths->length; index++)
    {
	char*     path  = paths->data[index];
	mt_map_t* entry = MGET(db, path);

	if (entry)
	{
	    changed |= lib_organize_entry(libpath, db, entry);
	}
    }

    REL(paths); // REL 0

    return changed;
}

int lib_comp_genre(void* left, void* right)
{
    char* la = MGET(left, "genre");
    char* ra = MGET(right, "genre");

    return strcmp(la, ra);
}

int lib_comp_artist(void* left, void* right)
{
    char* la = MGET(left, "artist");
    char* ra = MGET(right, "artist");

    return strcmp(la, ra);
}

void lib_get_genres(mt_vector_t* vec)
{
    int ei; // entry, genre index

    mt_vector_t* songs  = VNEW(); // REL 0
    mt_map_t*    genres = MNEW(); // REL 1

    mt_map_values(db, songs);

    for (ei = 0;
	 ei < songs->length;
	 ei++)
    {
	mt_map_t* entry = songs->data[ei];
	char*     genre = MGET(entry, "genre");

	if (genre)
	{
	    MPUT(genres, genre, entry);
	}
    }

    mt_map_values(genres, vec);
    mt_vector_sort(vec, lib_comp_genre);

    REL(genres); // REL 1
    REL(songs);  // REL 0
}

void lib_get_artists(mt_vector_t* vec)
{
    int ei;

    mt_vector_t* songs   = VNEW(); // REL 0
    mt_map_t*    artists = MNEW(); // REL 1

    mt_map_values(db, songs);

    for (ei = 0;
	 ei < songs->length;
	 ei++)
    {
	mt_map_t* entry  = songs->data[ei];
	char*     artist = MGET(entry, "artist");

	if (artist) MPUT(artists, artist, entry);
    }

    mt_map_values(artists, vec);
    mt_vector_sort(vec, lib_comp_artist);

    REL(artists); // REL 1
    REL(songs);   // REL 0
}

void lib_update_metadata(char* path, mt_map_t* changed, mt_vector_t* removed)
{
    mt_map_t*    song = MGET(db, path);
    mt_vector_t* keys = VNEW(); // REL 0
    mt_map_keys(changed, keys);

    // update changed

    for (int index = 0; index < keys->length; index++)
    {
	char* key = keys->data[index];
	MPUT(song, key, MGET(changed, key));
    }

    // remove removed

    for (int index = 0; index < removed->length; index++)
    {
	char* field = keys->data[index];
	MDEL(song, field);
    }

    REL(keys); // REL 0
}

#endif
