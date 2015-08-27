//
// Created by alex on 27/08/2015.
//

#include <lauxlib.h>
#include "larchive.h"
#include "lua.h"


static struct luaL_reg larchive[] = {

};

int LUA_LIBRARY lua_larchive(lua_State *L) {

    luaL_openlib(L, larchive, (sizeof(larchive) / sizeof(larchive[0])));

    return 0;
}