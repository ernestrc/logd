#include <errno.h>
#include <error.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "lua.h"
#include "logd_module.h"
#include "luv/luv.h"

static int lua_load_libs(lua_t* l)
{
	luaopen_logd(l->state);
	if (luaopen_luv(l->state) < 0)
		return 1;

	return 0;
}

static char* get_abs_path(char* path)
{
	static const char* ABS_TEMPLATE = "%s/%s";
	char cwd[1024];
	char* abs;
	if ((getcwd(cwd, sizeof(cwd))) == NULL) {
		perror("getcwd");
		return NULL;
	}

	int want = snprintf(NULL, 0, ABS_TEMPLATE, cwd, path);
	if ((abs = malloc(want)) == NULL) {
		perror("malloc");
		return NULL;
	}

	sprintf(abs, ABS_TEMPLATE, cwd, path);
	return abs;
}

static int lua_set_package_path(lua_t* l, const char* script)
{
	static const char* LUA_PATH_TEMPLATE = ";%s/?.lua";
	int need;
	int ret = 0;
	char* scriptdup = {0};
	char* path = {0};
	char* abs = {0};
	char* dir;

	if ((scriptdup = strdup(script)) == NULL) {
		perror("strdup");
		ret = -1;
		goto exit;
	}

	dir = dirname(scriptdup);
	if (strlen(dir) == 0) {
		errno = EINVAL;
		ret = -1;
		goto exit;
	}

	// is not absolute
	if (dir[0] != '/') {
		if ((abs = get_abs_path(dir)) == NULL) {
			perror("get_abs_path");
			ret = -1;
			goto exit;
		}
	} else if ((abs = strdup(dir)) == NULL) {
		perror("strdup");
		ret = -1;
		goto exit;
	}

	need = snprintf(NULL, 0, LUA_PATH_TEMPLATE, dir);
	path = malloc(need + 1);
	if (path == NULL) {
		perror("malloc");
		ret = -1;
		goto exit;
	}
	sprintf(path, LUA_PATH_TEMPLATE, dir);

	lua_getglobal(l->state, "package");
	lua_getfield(l->state, -1, "path");
	lua_pushstring(l->state, path);
	lua_concat(l->state, 2);
	lua_setfield(l->state, -2, "path");
	lua_pop(l->state, 1);

exit:
	if (scriptdup)
		free(scriptdup);
	if (path)
		free(path);
	if (abs)
		free(abs);
	return ret;
}

static void lua_push_on_log(lua_t* l)
{
	lua_getglobal(l->state, LUA_NAME_LOGD_MODULE);
	lua_getfield(l->state, -1, LUA_NAME_ON_LOG);
}

lua_t* lua_create(const char* script)
{
	lua_t* l = NULL;

	if ((l = calloc(1, sizeof(lua_t))) == NULL) {
		perror("calloc");
		goto error;
	}

	if (lua_init(l, script) == -1) {
		goto error;
	}

	return l;

error:
	free(l);
	return NULL;
}

int lua_init(lua_t* l, const char* script)
{
	int status;

	l->state = luaL_newstate();

	luaL_openlibs(l->state);
	if (lua_load_libs(l) != 0 || (l->loop = luv_loop(l->state)) == NULL) {
		perror("lua_load_libs");
		goto error;
	}

	if (lua_set_package_path(l, script) == -1) {
		goto error;
	}

	/* Load the script containing the script we are going to run */
	status = luaL_dofile(l->state, script);
	if (status) {
		fprintf(
		  stderr, "Couldn't load script: %s\n", lua_tostring(l->state, -1));
		errno = EINVAL;
		goto error;
	}

	lua_push_on_log(l);
	if (!lua_isfunction(l->state, -1)) {
		fprintf(stderr,
		  "Couldn not find '" LUA_NAME_LOGD_MODULE "." LUA_NAME_ON_LOG
		  "' function in loaded script\n");
		errno = EINVAL;
		goto error;
	}

	/* pop logd module and on_log from the stack */
	lua_pop(l->state, 2);

	return 0;

error:
	if (l->state)
		lua_close(l->state);
	return -1;
}

static int lua_lua_handler_error(lua_State* l)
{
	const char* err = lua_tostring(l, -1);
	fprintf(stderr, "runtime error: %s\n", err);
	// TODO
	// err, ok := l.ToString(-1)
	// if !ok {
	// 	panic(fmt.Errorf(
	// 		"no error in call to system error handler: found %s", l.TypeOf(-1)))
	// }
	// lua.Traceback(l, l, err, 1)
	// return 1
	return 1;
}

int lua_pcall_on_log(lua_t* l, log_t* log)
{
	int ret;

	// push error handler
	lua_pushcfunction(l->state, lua_lua_handler_error);
	DEBUG_ASSERT(lua_isfunction(l->state, -1));

	lua_push_on_log(l);
	DEBUG_ASSERT(lua_isfunction(l->state, -1));

	lua_pushlightuserdata(l->state, log);
	ret = lua_pcall(l->state, 1, 0, 1);

	if (ret == 0)
		lua_pop(l->state, 2); // pop error handler
	else
		lua_pop(l->state, 1); // logd module

	return ret;
}

void lua_call_on_log(lua_t* l, log_t* log)
{
	lua_push_on_log(l);
	DEBUG_ASSERT(lua_isfunction(l->state, -1));

	lua_pushlightuserdata(l->state, log);

	lua_call(l->state, 1, 0);
	lua_pop(l->state, 1); // logd module
}

// int lua_protected_on_log(lua_t* l, log_t* log)

void lua_free(lua_t* l)
{
	if (l)
		free(l);
}
