cmake_minimum_required(VERSION 3.3)
project(lua_libarchive)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(SOURCE_FILES
    larchive.c larchive.h)

include_directories(${LIBARCHIVE_INCLUDE_DIR} ${LUA_INCLUDE_DIR})
link_directories(${LIBARCHIVE_LIBRARY_DIR} ${LUA_LIBRARY_PATH})

add_library(larchive SHARED ${SOURCE_FILES})
target_link_libraries(larchive archive)

# =============
set_target_properties(larchive PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
# =============