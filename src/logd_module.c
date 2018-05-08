#include "logd_module.h"

#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lualib.h"

static const struct luaL_Reg logd_functions[] = {
  {LUA_NAME_ON_LOG, NULL}, {NULL, NULL}};

int luaopen_logd(lua_State* L)
{
	luaL_register(L, LUA_NAME_LOGD_MODULE, logd_functions);
	return 1;
}
