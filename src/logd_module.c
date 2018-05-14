#include <stdbool.h>
#include <string.h>

#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lualib.h"

#include "log.h"
#include "logd_module.h"
#include "util.h"

#define LUA_NAME_PRINT "print"
#define LUA_LEGACY_NAME_DEBUG "debug"
#define LUA_NAME_LOG_GET "log_get"
#define LUA_NAME_LOG_SET "log_set"
#define LUA_NAME_LOG_REMOVE "log_remove"
#define LUA_NAME_LOG_RESET "log_reset"
#define LUA_NAME_TABLE_TO_LOGPTR "to_logptr"
#define LUA_NAME_LOG_TO_STR "to_str"
// #define LUA_NAME_TO_LOG_JSON "to_json"

#define MAX_LOG_TABLE_LEN 25

#define ASSERT_LOG_PTR(L, idx, fn_name)                                        \
	switch (lua_type(L, idx)) {                                                \
	case LUA_TLIGHTUSERDATA:                                                   \
	case LUA_TUSERDATA:                                                        \
		break;                                                                 \
	default:                                                                   \
		luaL_error(L,                                                          \
		  "1st argument must be a logptr in call to '" fn_name "': found %s",  \
		  lua_typename(L, lua_type(L, idx)));                                  \
		break;                                                                 \
	}

static void table_to_log(
  lua_State* L, int idx, prop_t* props, int props_len, log_t* log)
{
	int added_props;
	const char* key;
	const char* value;
	bool level_added = false;
	bool time_added = false;
	bool date_added = false;

	for (lua_pushnil(L), added_props = 0; lua_next(L, idx); added_props++) {
		if (added_props >= props_len) {
			luaL_error(L,
			  "table exceeds max table len of %d in call to table_to_log",
			  props_len);
			return;
		}
		key = lua_tostring(L, -2);
		if (key == NULL) {
			luaL_error(L, "table key must be a string in call to table_to_log");
			return;
		}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored                                                 \
  "-Wincompatible-pointer-types-discards-qualifiers"
		sanitize_prop_key(key);
#pragma GCC diagnostic pop

		if (!level_added && strcmp(key, KEY_LEVEL) == 0)
			level_added = true;
		else if (!time_added && strcmp(key, KEY_TIME) == 0)
			time_added = true;
		else if (!date_added && strcmp(key, KEY_DATE) == 0)
			date_added = true;

		value = lua_tostring(L, -1);
		if (value == NULL) {
			luaL_error(
			  L, "table value must be a string in call to table_to_log");
			return;
		}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored                                                 \
  "-Wincompatible-pointer-types-discards-qualifiers"
		sanitize_prop_value(value);
#pragma GCC diagnostic pop

		log_set(log, &props[added_props], key, value);

		lua_pop(L, 1);
	}

	if (!level_added) {
		log_set(log, &props[added_props++], KEY_LEVEL, "DEBUG");
	}
	if (!time_added) {
		log_set(log, &props[added_props++], KEY_TIME, util_get_time());
	}
	if (!date_added) {
		log_set(log, &props[added_props], KEY_DATE, util_get_date());
	}
}

/*
 * logd_table_to_logptr will convert a table into a logptr userdata.
 * The returned pointer will be valid until the original table is GC'd.
 *
 * TODO: use weak tables to prevent props from being GCd
 */
static int logd_table_to_logptr(lua_State* L)
{
	log_t* log = (log_t*)lua_newuserdata(L, sizeof(log_t));
	prop_t* props =
	  (prop_t*)lua_newuserdata(L, MAX_LOG_TABLE_LEN * sizeof(prop_t));
	log_init(log);

	switch (lua_type(L, 1)) {
	case LUA_TTABLE:
		table_to_log(L, 1, props, MAX_LOG_TABLE_LEN, log);
		break;
	default:
		luaL_error(L,
		  "1st argument must be a table in call to '" LUA_NAME_TABLE_TO_LOGPTR
		  "': found %s",
		  lua_typename(L, lua_type(L, 1)));
		break;
	}

	return 2;
}

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

static void table_to_log_str(lua_State* L, int idx)
{
	prop_t props[MAX_LOG_TABLE_LEN];
	log_t log;
	log_init(&log);

	table_to_log(L, idx, props, MAX_LOG_TABLE_LEN, &log);

	char* str = force_snprintl(L, &log);
	lua_pushstring(L, str);

	free(str);
}

static int logd_log_to_str(lua_State* L)
{
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_TO_STR);

	log_t* log = (log_t*)lua_touserdata(L, 1);
	char* str = force_snprintl(L, log);
	lua_pushstring(L, str);
	free(str);

	return 1;
}

static int logd_print(lua_State* L)
{
	switch (lua_type(L, 1)) {
	case LUA_TTABLE:
		lua_getglobal(L, "print");
		table_to_log_str(L, 1);
		lua_call(L, 1, 0);
		break;
	case LUA_TSTRING:
		lua_newtable(L);
		lua_pushliteral(L, "msg");
		lua_pushvalue(L, 1);
		lua_settable(L, 2);
		lua_getglobal(L, "print");
		table_to_log_str(L, 2);
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

/*
 * TODO: use weak tables to prevent props from being GCd
 */
static int logd_log_set(lua_State* L)
{
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_TO_STR);
	log_t* log = (log_t*)lua_touserdata(L, 1);
	const char* key = lua_tostring(L, 2);
	if (key == NULL) {
		luaL_error(L,
		  "2nd argument must be a string in call to '" LUA_NAME_LOG_SET
		  "': found %s",
		  lua_typename(L, lua_type(L, 2)));
	}
	const char* value = lua_tostring(L, 3);
	if (value == NULL) {
		luaL_error(L,
		  "3rd argument must be a string in call to '" LUA_NAME_LOG_SET
		  "': found %s",
		  lua_typename(L, lua_type(L, 3)));
	}

	prop_t* prop = lua_newuserdata(L, sizeof(prop_t));
	log_set(log, prop, key, value);
	return 1;
}

static int logd_log_remove(lua_State* L)
{
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_TO_STR);
	log_t* log = (log_t*)lua_touserdata(L, 1);
	const char* key = lua_tostring(L, 2);
	if (key == NULL) {
		luaL_error(L,
		  "2nd argument must be a string in call to '" LUA_NAME_LOG_REMOVE
		  "': found %s",
		  lua_typename(L, lua_type(L, 2)));
	}

	log_remove(log, key);

	return 0;
}

static int logd_log_get(lua_State* L)
{
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_TO_STR);
	log_t* log = (log_t*)lua_touserdata(L, 1);
	const char* key = lua_tostring(L, 2);
	if (key == NULL) {
		luaL_error(L,
		  "2nd argument must be a string in call to '" LUA_NAME_LOG_GET
		  "': found %s",
		  lua_typename(L, lua_type(L, 2)));
	}
	const char* value = log_get(log, key);
	if (value != NULL)
		lua_pushstring(L, value);
	else
		lua_pushnil(L);

	return 1;
}

static int logd_log_reset(lua_State* L)
{
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_RESET);
	log_t* log = (log_t*)lua_touserdata(L, 1);

	log_init(log);

	return 0;
}

static const struct luaL_Reg logd_functions[] = {{LUA_NAME_ON_LOG, NULL},
  {LUA_NAME_PRINT, &logd_print}, {LUA_LEGACY_NAME_DEBUG, &logd_print},
  {LUA_NAME_TABLE_TO_LOGPTR, &logd_table_to_logptr},
  {LUA_NAME_LOG_TO_STR, &logd_log_to_str}, {LUA_NAME_LOG_GET, &logd_log_get},
  {LUA_NAME_LOG_SET, &logd_log_set}, {LUA_NAME_LOG_REMOVE, &logd_log_remove},
  {LUA_NAME_LOG_RESET, &logd_log_reset},
  {NULL, NULL}};

int luaopen_logd(lua_State* L)
{
	luaL_register(L, LUA_NAME_LOGD_MODULE, logd_functions);
	return 1;
}
