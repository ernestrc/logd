#ifndef LOGD_LUA_H
#define LOGD_LUA_H

#include "log.h"
#include "lua/lua.h"

typedef struct lua_s {
	lua_State* state;
} lua_t;

lua_t* lua_create(const char* script);
int lua_init(lua_t* l, const char* script);
void lua_call_on_log(lua_t*, log_t* log);
int lua_pcall_on_log(lua_t* l, log_t* log);
void lua_free(lua_t* l);

#endif
