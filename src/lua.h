#ifndef LOGD_LUA_H
#define LOGD_LUA_H

#include <stdbool.h>

#include "log.h"
#include "lua/lua.h"
#include "libuv/uv.h"

typedef struct lua_s {
	lua_State* state;
	uv_loop_t* loop;
} lua_t;

lua_t* lua_create(uv_loop_t* loop, const char* script);
int lua_init(lua_t* l, uv_loop_t* loop, const char* script);
void lua_call_on_log(lua_t*, log_t* log);
bool lua_on_eof_defined(lua_t*);
void lua_call_on_eof(lua_t*);
int lua_add_package_path(lua_State* state, const char* script);
void lua_free(lua_t* l);

#endif
