//
// Created by alex on 27/08/2015.
//

#ifndef LUA_LIBARCHIVE_LARCHIVE_H
#define LUA_LIBARCHIVE_LARCHIVE_H

#include <errno.h>

#ifdef _WIN32
#include <io.h>
#endif

#if defined(_WIN32) //  Microsoft
#define LUA_LIBRARY __declspec(dllexport)
#else //  Linux
#define LUA_LIBRARY __attribute__((visibility("default")))
#endif

#define LARCHIVE_NOERROR 0
#define LARCHIVE_ERROR   1
#define LARCHIVE_OK "OK"

struct archive_st {
    int code;
    const char *msg;
};

#define PATH_SEP '/'
#define UNZIP_DMODE 0755

/* Creates a new directory considering the execution platform */
int create_dir(const char *dir, mode_t mode) {
#ifdef __linux__
    return mkdir(dir, mode);
#else
    return mkdir(dir);
#endif
}

/* Create a directory tree based on the given path */
int mkdirs(const char *dir, mode_t mode) {
    if (!dir || strlen(dir) == 0)
        return -1;  // invalid directory.
    char *p;
    for (p = strchr(dir + 1, PATH_SEP); p; p = strchr(p + 1, PATH_SEP)) {
        *p = '\0';
        if (create_dir(dir, mode) == -1) {
            if (errno != EEXIST) {
                *p = PATH_SEP;
                return -1;
            }
        }
        *p = PATH_SEP;
    }
    return 0;
}

/* Joins two paths considering the platform sep (\\|/)*/
void join_path(char *dest, const char *path1, const char *path2) {
    if (path1 == NULL && path2 == NULL) {
        strcpy(dest, "");
    }
    else if (path2 == NULL || strlen(path2) == 0) {
        strcpy(dest, path1);
    }
    else if (path1 == NULL || strlen(path1) == 0) {
        strcpy(dest, path2);
    }
    else {
        char directory_separator[] = "/";
        const char *last_char = path1 + strlen(path1) - 1;
        int append_directory_separator = 0;

        if (strcmp(last_char, directory_separator) != 0) {
            append_directory_separator = 1;
        }
        strcpy(dest, path1);

        if (append_directory_separator) {
            strcat(dest, directory_separator);
        }
        strcat(dest, path2);
    }
}

int LUA_LIBRARY lua_larchiveopen(lua_State *L);

#endif //LUA_LIBARCHIVE_LARCHIVE_H
