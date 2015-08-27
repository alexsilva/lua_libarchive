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

#endif //LUA_LIBARCHIVE_LARCHIVE_H
