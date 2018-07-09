#ifndef LOGD_COLLECTOR_H
#define LOGD_COLLECTOR_H

#include <slab/buf.h>
#include <uv.h>

#include "./lua.h"
#include "./parser.h"

typedef struct collector_s {
	uv_file infd;
	buf_t* buf;
	void* parser;
	lua_t* lua_state;
	parse_res_t (*parse_parser)(void*, char*, size_t);
	uv_poll_t* uv_poll_in_req;
	uv_loop_t* loop;
	const char* dlparser_path;
	void* dlparser_handle;
	const char* input_file;
	void (*free_parser)(void*);
	void* (*create_parser)();
	void (*reset_parser)(void*);
	bool exit;
} collector_t;

collector_t* collector_create(
  uv_loop_t* loop, const char* dlparser_path, const char* input_file);

int collector_init(collector_t* c, uv_loop_t* loop, const char* dlparser_path,
  const char* input_file);

void collector_deinit(collector_t* c);

void collector_free(collector_t* c);

#endif
