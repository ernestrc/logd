#ifndef LOGD_LUA_MODULE_H
#define LOGD_LUA_MODULE_H

#include <lua.h>

#define LUA_NAME_ON_LOG "on_log"
#define LUA_NAME_ON_EXIT "on_exit"
#define LUA_NAME_ON_ERROR "on_error"
#define LUA_NAME_LOGD_MODULE "logd"

enum exit_reason {
	REASON_ERROR = 0,
	REASON_SIGNAL = 1,
	REASON_EOF = 2,
};

LUALIB_API int luaopen_logd(lua_State* L);

#endif
