#ifndef LOGD_LUA_MODULE_H
#define LOGD_LUA_MODULE_H

#include <lua.h>

#define LUA_NAME_ON_LOG "on_log"
#define LUA_NAME_ON_EOF "on_eof"
#define LUA_NAME_ON_ERROR "on_error"
#define LUA_NAME_LOGD_MODULE "logd"

LUALIB_API int luaopen_logd(lua_State* L);

#endif
