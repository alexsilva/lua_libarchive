//
// Created by alex on 27/08/2015.
//

#include <lauxlib.h>
#include <archive.h>
#include <stdlib.h>
#include <archive_entry.h>
#include "larchive.h"
#include "lua.h"

static int write_data(struct archive *arch, struct archive *arch_writer, struct archive_extraction *arch_extraction) {
    int retval;
    const void *buff;
    size_t size;
    int64_t offset;
    while (1) {
        retval = archive_read_data_block(arch, &buff, &size, &offset);
        if (retval == ARCHIVE_EOF) {
            arch_extraction->code = ARCHIVE_OK;
            return 1;
        }
        if (retval < ARCHIVE_OK) {
            arch_extraction->code = retval;
            arch_extraction->msg = archive_error_string(arch);
            return retval < ARCHIVE_WARN ? 0 : 1; // 0 == error
        }
        retval = archive_write_data_block(arch_writer, buff, size, offset);
        if (retval < ARCHIVE_OK) {
            arch_extraction->code = retval;
            arch_extraction->msg = archive_error_string(arch_writer);
            return retval < ARCHIVE_WARN ? 0 : 1;
        }
    }
}

static void larchive_extract(lua_State *L) {
    char *filename = luaL_check_string(L, 1);

    struct archive *arch;
    struct archive *arch_writer;
    struct archive_entry *entry;
    struct archive_extraction arch_extraction;

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
    arch_extraction.code = ARCHIVE_EXT_UNDEFINED;
    arch_extraction.msg  = "undefined";

    if ((retval = archive_read_open_filename(arch, filename, 10240)) != 0) {
        arch_extraction.msg = archive_error_string(arch);
        arch_extraction.code = ARCHIVE_EXT_ERROR;

        // lua push status
        lua_pushnumber(L, arch_extraction.code);
        lua_pushstring(L, (char *) arch_extraction.msg);
        return; // fatal error
    }
    while (1) {
        retval = archive_read_next_header(arch, &entry);
        if (retval == ARCHIVE_EOF) {
            arch_extraction.code = ARCHIVE_EXT_SUCCESS;
            arch_extraction.msg = "OK";
            break;
        }
        if (retval < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(arch));
        }
        if (retval < ARCHIVE_WARN) {
            arch_extraction.msg = archive_error_string(arch);
            arch_extraction.code = retval;
            break;
        }
        retval = archive_write_header(arch_writer, entry);
        if (retval < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(arch_writer));
        }
        else if (archive_entry_size(entry) > 0 && !write_data(arch, arch_writer, &arch_extraction)) {
            break;  // error writing data...
        }
        retval = archive_write_finish_entry(arch_writer);
        if (retval < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(arch_writer));
        }
        if (retval < ARCHIVE_WARN) {
            arch_extraction.msg = archive_error_string(arch_writer);
            arch_extraction.code = retval;
            break;
        }
    }
    archive_read_close(arch);
    archive_read_free(arch);
    archive_write_close(arch_writer);
    archive_write_free(arch_writer);

    // lua push status
    lua_pushnumber(L, arch_extraction.code);
    lua_pushstring(L, (char *) arch_extraction.msg);
}

static struct luaL_reg larchive[] = {
    {"archive_extract", larchive_extract},
};

int LUA_LIBRARY lua_larchiveopen(lua_State *L) {

    luaL_openlib(L, larchive, (sizeof(larchive) / sizeof(larchive[0])));

    return 0;
}