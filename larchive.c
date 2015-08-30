//
// Created by alex on 27/08/2015.
//

#include <lauxlib.h>
#include <archive.h>
#include <stdlib.h>
#ifdef __linux__
#include <linux/limits.h>
#endif
#ifndef lua_next
#include "lapi_missing.h"
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif
#include <archive_entry.h>
#include <fcntl.h>
#include <libgen.h>
#include "larchive.h"
#include "lua.h"


char *string_copy(const char *str) {
    size_t len = strlen(str);
    char *x = (char *) malloc(len + 1); /* 1 for the null terminator */
    if(!x) return NULL; /* malloc could not allocate memory */
    memcpy(x,str,len+1); /* copy the string into the new buffer */
    return x;
}

static struct archive * get_archive_ref(lua_State *L, int pos) {
    lua_Object lobj = lua_getparam(L, pos);
    if (!lua_isuserdata(L, lobj)) {
        lua_error(L, "archive ref(1) is required!");
    }
    return lua_getuserdata(L, lobj);
}


static int write_data(struct archive *arch, struct archive *arch_writer, struct archive_st *arch_st) {
    int retval;
    const void *buff;
    size_t size;
    int64_t offset;
    while (1) {
        retval = archive_read_data_block(arch, &buff, &size, &offset);
        if (retval == ARCHIVE_EOF) {
            arch_st->code = ARCHIVE_OK;
            return 1;
        }
        if (retval < ARCHIVE_OK) {
            arch_st->code = retval;
            arch_st->msg = archive_error_string(arch);
            return retval < ARCHIVE_WARN ? 0 : 1; // 0 == error
        }
        retval = archive_write_data_block(arch_writer, buff, size, offset);
        if (retval < ARCHIVE_OK) {
            arch_st->code = retval;
            arch_st->msg = archive_error_string(arch_writer);
            return retval < ARCHIVE_WARN ? 0 : 1;
        }
    }
}

static void archive_entry_change_dir(char *basedir, struct archive *arch, struct archive_entry *entry) {
    const char* path = archive_entry_pathname( entry );

    char filepath[PATH_MAX + 1];
    join_path(&filepath[0], basedir, path);

    archive_entry_set_pathname(entry, filepath);
}

static void larchive_extract(lua_State *L) {
    char *filename = luaL_check_string(L, 1);
    char *basedir = luaL_check_string(L, 2);

    struct archive *arch;
    struct archive *arch_writer;
    struct archive_entry *entry;
    struct archive_st arch_st;

    int flags;
    int retval;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    arch = archive_read_new();
    archive_read_support_format_all(arch);
    archive_read_support_filter_all(arch);

    arch_writer = archive_write_disk_new();
    archive_write_disk_set_options(arch_writer, flags);
    archive_write_disk_set_standard_lookup(arch_writer);

    // initial state
    arch_st.code = ARCHIVE_EXT_UNDEFINED;
    arch_st.msg  = "undefined";

    if (mkdirs(basedir, UNZIP_DMODE)) {
        arch_st.code = ARCHIVE_EXT_ERROR;
        arch_st.msg = "creating base directory";
        // lua push status
        lua_pushnumber(L, arch_st.code);
        lua_pushstring(L, (char *) arch_st.msg);
        return; // fatal error
    }
    if ((retval = archive_read_open_filename(arch, filename, 10240)) != 0) {
        arch_st.msg = archive_error_string(arch);
        arch_st.code = ARCHIVE_EXT_ERROR;

        // lua push status
        lua_pushnumber(L, arch_st.code);
        lua_pushstring(L, (char *) arch_st.msg);
        return; // fatal error
    }
    while (1) {
        retval = archive_read_next_header(arch, &entry);
        if (retval == ARCHIVE_EOF) {
            arch_st.code = ARCHIVE_EXT_SUCCESS;
            arch_st.msg = "OK";
            break;
        }
        if (retval < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(arch));
        }
        if (retval < ARCHIVE_WARN) {
            arch_st.msg = archive_error_string(arch);
            arch_st.code = retval;
            break;
        }

        archive_entry_change_dir(basedir, arch, entry);

        retval = archive_write_header(arch_writer, entry);
        if (retval < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(arch_writer));
        }
        else if (archive_entry_size(entry) > 0 && !write_data(arch, arch_writer, &arch_st)) {
            break;  // error writing data...
        }
        retval = archive_write_finish_entry(arch_writer);
        if (retval < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(arch_writer));
        }
        if (retval < ARCHIVE_WARN) {
            arch_st.msg = archive_error_string(arch_writer);
            arch_st.code = retval;
            break;
        }
    }
    archive_read_close(arch);
    archive_read_free(arch);
    archive_write_close(arch_writer);
    archive_write_free(arch_writer);

    // lua push status
    lua_pushnumber(L, arch_st.code);
    lua_pushstring(L, (char *) arch_st.msg);
}

static void lzip_open(lua_State *L) {
    char *filepath = luaL_check_string(L, 1);
    struct archive *arch;

    if (!(arch = archive_write_new())) {
        lua_pushnumber(L, -1);
        lua_pushstring(L, "unable to start a new instance of zip.");
        return; // fatal error
    }
    archive_write_add_filter_lzip(arch);
    archive_write_set_format_zip(arch);

    if (mkdirs(filepath, UNZIP_DMODE)) {  // base dir to zip
        lua_pushnumber(L, -1);
        lua_pushstring(L, "creating base directory");
        return; // fatal error
    }
    if (archive_write_open_filename(arch, filepath) != ARCHIVE_OK) {
        const char *error_string = archive_error_string(arch);
        int error_num = archive_errno(arch);

        lua_pushnumber(L, error_num);
        lua_pushstring(L, (char *) error_string);
        return; // fatal error
    }
    lua_pushnumber(L, 0);
    lua_pushuserdata(L, arch); // new file
}

static struct archive_st larchive_write_file(struct archive *arch, char *filename, char *filedest) {
    struct stat st;
    char buff[8192];
    struct archive_entry *entry;
    struct archive_st arch_st;
    int len;
    int fd;

    stat(filename, &st);

    arch_st.msg = "OK";
    arch_st.code = 0;

    char *flname = NULL;
    char *zip_basename = NULL;

    if (!filedest) {
        flname = string_copy(filename);
        if (!flname) {
            arch_st.code = -1;
            arch_st.msg = "out of memory.";
            return arch_st;
        }
        zip_basename = basename(flname);
        if (!zip_basename) {
            arch_st.code = -1;
            arch_st.msg = "out of memory.";
            return arch_st;
        }
    } else {
        zip_basename = filedest;
    }
    entry = archive_entry_new(); // Note 2
    archive_entry_set_pathname(entry, zip_basename);
    archive_entry_set_size(entry, st.st_size); // Note 3
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_write_header(arch, entry);

    if ((fd = open(filename, O_RDONLY | O_BINARY)) == -1) {
        if (flname) free(flname); // free!
        arch_st.code = errno;
        arch_st.msg = strerror(errno);
        return arch_st;
    }
    len = read(fd, buff, sizeof(buff));

    while ( len > 0 ) {
        archive_write_data(arch, buff, (size_t) len);
        len = read(fd, buff, sizeof(buff));
    }
    close(fd);
    archive_entry_free(entry);
    if (flname) free(flname); // free!
    return arch_st;
}

/* Lua archive close */
static void lzip_close(lua_State *L) {
    struct archive *arch = get_archive_ref(L, 1);
    const char *msg = "OK";
    int error_num = 0;

    int close_code = archive_write_close(arch);
    int free_code = archive_write_free(arch);

    if (close_code != ARCHIVE_OK || free_code != ARCHIVE_OK) {// error check!
        msg = archive_error_string(arch);
        error_num = archive_errno(arch);
    }
    lua_pushnumber(L, error_num);
    lua_pushstring(L, (char *) msg);
}

static void lzip_add(lua_State *L) {
    struct archive *arch = get_archive_ref(L, 1);
    char *filepath = luaL_check_string(L, 2);

    // path relative, inside zip
    lua_Object lobj = lua_getparam(L, 3);
    char *filedest  = lua_getstring(L, lobj);

    struct archive_st arch_st = larchive_write_file(arch, filepath, filedest);

    lua_pushnumber(L, arch_st.code);
    lua_pushstring(L, (char *) arch_st.msg);
}


static struct luaL_reg larchive[] = {
    {"archive_extract", larchive_extract},
    {"zip_open", lzip_open},
    {"zip_add", lzip_add},
    {"zip_close", lzip_close}
};

int LUA_LIBRARY lua_larchiveopen(lua_State *L) {

    luaL_openlib(L, larchive, (sizeof(larchive) / sizeof(larchive[0])));

    return 0;
}