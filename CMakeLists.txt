cmake_minimum_required(VERSION 3.3)
project(lua_libarchive)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(SOURCE_FILES
    larchive.c larchive.h lapi_missing.h lapi_missing.c lapi_missing.h)

include_directories(${LIBARCHIVE_INCLUDE_DIR} ${LUA_INCLUDE_DIR} ${LUA_INCLUDE_EXTRA})
link_directories(${LIBARCHIVE_LIBRARY_DIR})

add_library(larchive SHARED ${SOURCE_FILES} lapi_missing.h lapi_missing.c lapi_missing.h)
target_link_libraries(larchive archive ${LUA_LIBRARY_PATH})

# =============
set_target_properties(larchive PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
# =============