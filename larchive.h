//
// Created by alex on 27/08/2015.
//

#ifndef LUA_LIBARCHIVE_LARCHIVE_H
#define LUA_LIBARCHIVE_LARCHIVE_H

#if defined(_WIN32) //  Microsoft
#define LUA_LIBRARY __declspec(dllexport)
#else //  Linux
#define LUA_LIBRARY __attribute__((visibility("default")))
#endif

#define ARCHIVE_EXT_ERROR      0
#define ARCHIVE_EXT_SUCCESS    1
#define ARCHIVE_EXT_UNDEFINED -1


struct archive_extraction {
    int code;
    const char *msg;
};

int LUA_LIBRARY lua_larchiveopen(lua_State *L);

#endif //LUA_LIBARCHIVE_LARCHIVE_H
