#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/lua.h"
#include "../src/lua/lauxlib.h"
#include "../src/lua/lua.h"
#include "../src/lua/lualib.h"

struct args_s {
	int help;
} args;

struct test_s {
	struct test_s* next;
	char* desc;
} tests;

void args_init(int argc, char* argv[], char** script)
{
	/* set defaults for arguments */
	args.help = 0;

	static struct option long_options[] = {
	  {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	while (
	  (c = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			args.help = 1;
			break;
		default:
			abort();
		}
	}

	*script = argv[optind];
}


// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
// TODO ditch this and use https://www.mroth.net/lunit/ instead
void print_usage(const char* exe)
{
	printf("usage: %s <script> [options]\n", exe);
	printf("\noptions:\n");
	printf("\t-h, --help          prints this message\n");
}

int main(int argc, char* argv[])
{
	int ret = 0;
	char* script;
	lua_State* state;

	args_init(argc, argv, &script);
	state = luaL_newstate();
	luaL_openlibs(state);
	lua_add_package_path(state, script);
	if ((ret = luaL_dofile(state, script)) != 0) {
		fprintf(
		  stderr, "Couldn't load lua tests: %s\n", lua_tostring(state, -1));
		goto exit;
	}

	ret = luaL_dostring(state,
	  "for k in pairs(_G) do\n"
	  "if string.sub(k,1,5)==\"test_\" then\n"
	  "_G[k]()\n"
	  "end\n"
	  "end\n");
exit:
	lua_close(state);
	return ret;
}
