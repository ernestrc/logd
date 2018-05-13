#include <stdbool.h>
#include <string.h>

#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lualib.h"

#include "log.h"
#include "logd_module.h"
#include "util.h"

#define MAX_PRINT_TABLE_LEN 25
#define LUA_NAME_PRINT "print"
#define LUA_LEGACY_NAME_DEBUG "debug"

static char* force_snprintl(lua_State* L, log_t* log)
{
	int size = snprintl(NULL, 0, log);
	int strlen = size + 1;
	char* str = malloc(strlen);
	if (str == NULL) {
		luaL_error(L, "force_snprintl: ENOMEM");
		return NULL;
	}
	snprintl(str, strlen, log);

	return str;
}

static void push_log_table(lua_State* L, int idx)
{
	int base_props, all_props, added_props;
	const char* key;
	const char* value;
	log_t log;
	prop_t props[MAX_PRINT_TABLE_LEN];
	bool level_added = false;

	log_init(&log);

	all_props = base_props =
	  log_set_base_prop(&log, props, MAX_PRINT_TABLE_LEN);

	for (lua_pushnil(L), added_props = 0; lua_next(L, idx);
		 added_props++, all_props++) {
		if (all_props >= MAX_PRINT_TABLE_LEN) {
			luaL_error(L,
			  "table exceeds max table len of %d in call to '" LUA_NAME_PRINT
			  "'",
			  MAX_PRINT_TABLE_LEN - base_props);
			return;
		}
		key = lua_tostring(L, -2);
		if (key == NULL) {
			luaL_error(
			  L, "table key must be a string in call to '" LUA_NAME_PRINT "'");
			return;
		}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
		sanitize_prop_key(key);
#pragma GCC diagnostic pop

		if (!level_added && strcmp(key, KEY_LEVEL) == 0)
			level_added = true;

		value = lua_tostring(L, -1);
		if (value == NULL) {
			luaL_error(L,
			  "table value must be a string in call to '" LUA_NAME_PRINT "'");
			return;
		}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
		sanitize_prop_value(value);
#pragma GCC diagnostic pop

		log_set(&log, &props[all_props], key, value);

		// push key back in front for lua_next but
		// keep original key and value in the stack until
		// we have serialized log
		lua_pushvalue(L, -2);
	}

	if (!level_added) {
		log_set(&log, &props[all_props], KEY_LEVEL, "DEBUG");
	}

	char* str = force_snprintl(L, &log);
	lua_pop(L, added_props * 2);
	lua_pushstring(L, str);
	free(str);
}

static int logd_print(lua_State* L)
{
	switch (lua_type(L, 1)) {
	case LUA_TTABLE:
		lua_getglobal(L, "print");
		push_log_table(L, 1);
		lua_call(L, 1, 0);
		break;
	case LUA_TSTRING:
		lua_newtable(L);
		lua_pushliteral(L, "msg");
		lua_pushvalue(L, 1);
		lua_settable(L, 2);
		lua_getglobal(L, "print");
		push_log_table(L, 2);
		lua_call(L, 1, 0);
		lua_pop(L, 1);
		break;
	default:
		luaL_error(L,
		  "1st argument must be a string or a table in call to '" LUA_NAME_PRINT
		  "': found %s",
		  lua_typename(L, lua_type(L, 1)));
		break;
	}

	return 0;
}

static const struct luaL_Reg logd_functions[] = {{LUA_NAME_ON_LOG, NULL},
  {LUA_NAME_PRINT, &logd_print}, {LUA_LEGACY_NAME_DEBUG, &logd_print},
  {NULL, NULL}};

int luaopen_logd(lua_State* L)
{
	luaL_register(L, LUA_NAME_LOGD_MODULE, logd_functions);
	return 1;
}
