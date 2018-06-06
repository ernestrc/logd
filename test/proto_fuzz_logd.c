#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/log.h"
#include "../src/lua.h"
#include "../src/parser.h"
#include "../src/slab/buf.h"

extern void* parser;
extern void (*free_parser)(void*);
extern void* (*create_parser)();
extern void (*reset_parser)(void*);
extern parse_res_t (*parse_parser)(void*, char*, size_t);
extern void release_all();
extern buf_t* b;
extern uv_loop_t* loop;
extern lua_t* lstate;

#define BUF_INIT_CAP 1000

static char* data_copy;

int LLVMFuzzerInitialize(int* argc, char*** argv)
{
	int ret;

	if ((parser = create_parser()) == NULL) {
		perror("parser_create");
		release_all();
		return 1;
	}

	if ((b = buf_create(BUF_INIT_CAP)) == NULL) {
		perror("buf_create");
		release_all();
		return 1;
	}

	if ((lstate = lua_create(loop, "")) == NULL) {
		perror("lua_create");
		release_all();
		return 1;
	}

	return 0;
}

void on_read(uv_fs_t* req);

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	int ret = 0;
	uv_fs_t req;
	req.result = size;
	buf_write(b, data, size);
	on_read(&req);

end:
	release_all();
	return ret;
	return 0;
}
