//
// Created by alex on 27/08/2015.
//

#include <lauxlib.h>
#include <archive.h>
#include <stdlib.h>
#include <archive_entry.h>
#include "larchive.h"
#include "lua.h"

static int write_data(struct archive *ar, struct archive *aw) {
    int r;
    const void *buff;
    size_t size;
    int64_t offset;
    while (1) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
            return (ARCHIVE_OK);
        if (r < ARCHIVE_OK)
            return (r);
        r = archive_write_data_block(aw, buff, size, offset);
        if (r < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(aw));
            return (r);
        }
    }
}

static void larchive_extract(lua_State *L) {
    char *filename = luaL_check_string(L, 1);

    struct archive *arch;
    struct archive *arch_writer;
    struct archive_entry *entry;
    int flags;
    int r;

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

    if ((r = archive_read_open_filename(arch, filename, 10240)) != 0)
        exit(1);

    while (1) {
        r = archive_read_next_header(arch, &entry);
        if (r == ARCHIVE_EOF)
            break;

        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(arch));
        if (r < ARCHIVE_WARN)
            exit(1);

        r = archive_write_header(arch_writer, entry);
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(arch_writer));

        else if (archive_entry_size(entry) > 0) {
            r = write_data(arch, arch_writer);
            if (r < ARCHIVE_OK)
                fprintf(stderr, "%s\n", archive_error_string(arch));
            if (r < ARCHIVE_WARN)
                exit(1);
        }
        r = archive_write_finish_entry(arch_writer);
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(arch_writer));

        if (r < ARCHIVE_WARN)
            exit(1);
    }
    archive_read_close(arch);
    archive_read_free(arch);
    archive_write_close(arch_writer);
    archive_write_free(arch_writer);
}

static struct luaL_reg larchive[] = {
    {"archive_extract", larchive_extract},
};

int LUA_LIBRARY lua_larchiveopen(lua_State *L) {

    luaL_openlib(L, larchive, (sizeof(larchive) / sizeof(larchive[0])));

    return 0;
}