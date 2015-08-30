//
// Created by alex on 30/08/2015.
//

#ifndef LUA_LIBARCHIVE_LAPI_MISSING_H
#define LUA_LIBARCHIVE_LAPI_MISSING_H

#include <lua.h>

#define lapi_address(L, lo) ((lo)+L->stack.stack-1)

int lapi_next(lua_State *L, lua_Object o, int i);

#define lua_next(state, obj, index) lapi_next(state, obj, index);

#endif //LUA_LIBARCHIVE_LAPI_MISSING_H
