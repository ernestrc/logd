#include <stdbool.h>
#include <string.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "config.h"
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
#define LUA_LEGACY_NAME_LOG_STRING "log_string"

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

		if (!level_added && strcmp(key, KEY_LEVEL) == 0)
			level_added = true;
		else if (!time_added && strcmp(key, KEY_TIME) == 0)
			time_added = true;
		else if (!date_added && strcmp(key, KEY_DATE) == 0)
			date_added = true;

		switch (lua_type(L, -1)) {
		case LUA_TNONE:
		case LUA_TNIL:
			value = "nil";
			break;
		case LUA_TBOOLEAN:
			value = lua_toboolean(L, -1) ? "true" : "false";
			break;
		case LUA_TNUMBER:
		case LUA_TSTRING:
			value = lua_tostring(L, -1);
			break;
		case LUA_TTABLE:
			value = "<table>";
			break;
		case LUA_TFUNCTION:
			value = "<func>";
			break;
		case LUA_TLIGHTUSERDATA:
		case LUA_TUSERDATA:
			value = "<ptr>";
			break;
		case LUA_TTHREAD:
			value = "<thread>";
			break;
		default:
			abort(); /* not possible */
		}

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
static int table_to_logptr(lua_State* L, int idx)
{
	log_t* log = (log_t*)lua_newuserdata(L, sizeof(log_t));
	prop_t* props =
	  (prop_t*)lua_newuserdata(L, LOGD_PRINT_MAX_KEYS * sizeof(prop_t));
	log_init(log);

	switch (lua_type(L, idx)) {
	case LUA_TTABLE:
		table_to_log(L, idx, props, LOGD_PRINT_MAX_KEYS, log);
		break;
	default:
		luaL_error(L,
		  "1st argument must be a table in call to '" LUA_NAME_TABLE_TO_LOGPTR
		  "': found %s",
		  lua_typename(L, lua_type(L, idx)));
		break;
	}

	return 2;
}

static int logd_table_to_logptr(lua_State* L)
{
	return table_to_logptr(L, 1);
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

static int logd_log_to_str(lua_State* L)
{
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_TO_STR);

	log_t* log = (log_t*)lua_touserdata(L, 1);
	char* str = force_snprintl(L, log);
	lua_pushstring(L, str);
	free(str);

	return 1;
}

void lua_print_log(lua_State* L, int idx)
{
	ASSERT_LOG_PTR(L, idx, LUA_NAME_PRINT);
	log_t* log = (log_t*)lua_touserdata(L, idx);
	char* str = force_snprintl(L, log);
	lua_getglobal(L, "io");
	lua_getfield(L, -1, "write");
	lua_pushstring(L, str);
	lua_call(L, 1, 0);
	lua_getfield(L, -1, "write");
	lua_pushliteral(L, "\n");
	lua_call(L, 1, 0);
	lua_pop(L, 1);
	free(str);
}

static int logd_print(lua_State* L)
{
	switch (lua_type(L, 1)) {
	case LUA_TUSERDATA:
	case LUA_TLIGHTUSERDATA:
		lua_print_log(L, 1);
		break;
	case LUA_TTABLE:
		logd_table_to_logptr(L);
		lua_print_log(L, 2);
		lua_pop(L, 2);
		break;
	case LUA_TSTRING:
		lua_newtable(L);
		lua_pushliteral(L, "msg");
		lua_pushvalue(L, 1);
		lua_settable(L, 2);
		table_to_logptr(L, 2);
		lua_print_log(L, 3);
		lua_pop(L, 3);
		break;
	default:
		luaL_error(L,
		  "1st argument must be a string, a table or a logptr in call to '" LUA_NAME_PRINT
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
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_SET);
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
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_REMOVE);
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
	ASSERT_LOG_PTR(L, 1, LUA_NAME_LOG_GET);
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
  {LUA_NAME_LOG_TO_STR, &logd_log_to_str},
  {LUA_LEGACY_NAME_LOG_STRING, &logd_log_to_str},
  {LUA_NAME_LOG_GET, &logd_log_get}, {LUA_NAME_LOG_SET, &logd_log_set},
  {LUA_NAME_LOG_REMOVE, &logd_log_remove},
  {LUA_NAME_LOG_RESET, &logd_log_reset}, {NULL, NULL}};

LUALIB_API int luaopen_logd(lua_State* L)
{
	luaL_register(L, LUA_NAME_LOGD_MODULE, logd_functions);
	return 1;
}
