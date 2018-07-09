#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <uv.h>

#include "./collector.h"
#include "./config.h"
#include "./util.h"

static void on_read(uv_poll_t* req, int status, int events);

static void on_close_lua_handle(uv_handle_t* handle)
{
	DEBUG_LOG("closed uv handle %p", handle);
}

static void uv_walk_close_lua_handles(uv_handle_t* handle, void* arg)
{
	collector_t* c = (collector_t*)arg;

	if (handle->data == c || uv_is_closing(handle)) {
		DEBUG_LOG("skipping uv handle %p", handle);
		return;
	}

	DEBUG_LOG("closing uv handle %p", handle);
	uv_close(handle, on_close_lua_handle);
}

void collector_close_lua_handles(collector_t* c)
{
	uv_walk(c->loop, uv_walk_close_lua_handles, c);
}

static void close_collector_handles(collector_t* c)
{
	uv_fs_t req_in_close;
	uv_fs_close(c->loop, &req_in_close, c->infd, NULL);
	uv_fs_req_cleanup(&req_in_close);
	uv_poll_stop(c->uv_poll_in_req);
}

void collector_close_all_handles(collector_t* c)
{
	collector_close_lua_handles(c);
	close_collector_handles(c);
}

static void on_uv_err(collector_t* c, int ret)
{
	errno = -ret;
	perror("uv");
	collector_close_all_handles(c);
	// TODO mark collector as done for logd.c
}

static void on_err(collector_t* c)
{
	perror("input read");
	collector_close_all_handles(c);
	// TODO mark collector as done for logd.c
}

void on_eof(collector_t* c)
{
	if (lua_on_eof_defined(c->lua_state)) {
		lua_call_on_eof(c->lua_state);
	}
	close_collector_handles(c);
	// TODO mark collector as done for logd.c
}

static void on_read_eof(collector_t* c)
{
	parse_res_t res;
	DEBUG_LOG("EOF while reading input file %d", c->infd);

parse:
	res = c->parse_parser(c->parser, c->buf->next_read, buf_readable(c->buf));
	switch (res.type) {
	case PARSE_COMPLETE:
		buf_ack(c->buf, res.consumed);
		lua_call_on_log(c->lua_state, res.log);
		c->reset_parser(c->parser);
		goto parse;
	case PARSE_ERROR:
		DEBUG_LOG("EOF parse error: %s", res.error.msg);
		buf_ack(c->buf, res.consumed);
		if (lua_on_error_defined(c->lua_state)) {
			lua_call_on_error(
			  c->lua_state, res.error.msg, res.log, res.error.at);
		}
		c->reset_parser(c->parser);
		goto parse;
	case PARSE_PARTIAL:
		on_eof(c);
		break;
	}

	return;
}

static void on_read_skip(uv_poll_t* req, int status, int events)
{
	collector_t* c = (collector_t*)req->data;
	parse_res_t res;
	if (status < 0) {
		on_uv_err(c, status);
		return;
	}

	errno = 0;
	int ret = read(c->infd, c->buf->next_write, buf_writable(c->buf));
	if (errno == EAGAIN) {
		DEBUG_LOG("input not ready: %d", c->infd);
		return;
	}
	if (ret < 0) {
		on_err(c);
		return;
	}

	if (ret == 0) {
		on_eof(c);
		return;
	}

	buf_extend(c->buf, ret);
	res = c->parse_parser(c->parser, c->buf->next_read, buf_readable(c->buf));
	if (res.type == PARSE_PARTIAL) {
		buf_reset_offsets(c->buf);
		return;
	}

	if (lua_on_error_defined(c->lua_state)) {
		lua_call_on_error(c->lua_state,
		  "log line was skipped because it is more than " STR(
			LOGD_BUF_MAX_CAP) " bytes",
		  res.log, "");
	}
	buf_ack(c->buf, res.consumed);
	DEBUG_LOG("successfully skipped line: buffer has now %zu readable bytes "
			  "and %zu writable bytes",
	  buf_readable(c->buf), buf_writable(c->buf));
	c->reset_parser(c->parser);

	if ((ret = uv_poll_stop(c->uv_poll_in_req)) < 0 ||
	  (ret = uv_poll_start(c->uv_poll_in_req, UV_READABLE, &on_read)) < 0) {
		on_uv_err(c, ret);
	}
}

static void on_read(uv_poll_t* req, int status, int events)
{
	collector_t* c = (collector_t*)req->data;
	parse_res_t res;
	if (status < 0) {
		on_uv_err(c, status);
		return;
	}

	errno = 0;
	int ret = read(c->infd, c->buf->next_write, buf_writable(c->buf));
	if (errno == EAGAIN) {
		DEBUG_LOG("input not ready: %d", c->infd);
		return;
	}
	if (ret < 0) {
		on_err(c);
		return;
	}

	if (ret == 0) {
		on_read_eof(c);
		return;
	}

	buf_extend(c->buf, ret);
parse:
	res = c->parse_parser(c->parser, c->buf->next_read, buf_readable(c->buf));
	switch (res.type) {

	case PARSE_COMPLETE:
		lua_call_on_log(c->lua_state, res.log);
		buf_consume(c->buf, res.consumed);
		c->reset_parser(c->parser);
		goto parse;

	case PARSE_ERROR:
		DEBUG_LOG("parse error: %s", res.error.msg);
		buf_ack(c->buf, res.consumed);
		if (lua_on_error_defined(c->lua_state)) {
			lua_call_on_error(
			  c->lua_state, res.error.msg, res.log, res.error.at);
		}
		c->reset_parser(c->parser);
		goto parse;

	case PARSE_PARTIAL:
		if (!buf_full(c->buf)) {
			buf_ack(c->buf, res.consumed);
			goto read;
		}

		if (buf_compact(c->buf)) {
			DEBUG_LOG(
			  "compacted buffer: now writable %zd bytes", buf_writable(c->buf));
			c->reset_parser(c->parser);
			goto read;
		}

		/* we have allocated too much memory, start skipping data */
		if (c->buf->cap > LOGD_BUF_MAX_CAP) {
			DEBUG_LOG("log is too long (more than %d bytes). skipping data...",
			  LOGD_BUF_MAX_CAP);
			buf_reset_offsets(c->buf);
			goto skip;
		}

		/* complete log doesn't fit buffer so reserve more space */
		/* do not ack so we re-parse with the re-allocated buffer */
		if (buf_reserve(c->buf, LOGD_BUF_INIT_CAP) != 0) {
			perror("buf_reserve");
			fprintf(stderr, "error reserving more space in input buffer\n");
			on_err(c);
			return;
		}

		DEBUG_LOG(
		  "reserved more space in input buffer: now %zd bytes", c->buf->cap);
		c->reset_parser(c->parser);
		goto read;
	}

read:
	return;
skip:
	if ((uv_poll_stop(c->uv_poll_in_req)) < 0 ||
	  (uv_poll_start(c->uv_poll_in_req, UV_READABLE, &on_read_skip)) < 0)
		on_uv_err(c, ret);
	return;
}

static int input_init(collector_t* c)
{
	int ret = 0;
	uv_fs_t uv_open_in_req = {0};

	if ((c->uv_poll_in_req = calloc(1, sizeof(uv_poll_t))) == NULL) {
		perror("calloc");
		ret = 1;
		goto exit;
	}

	if ((ret = uv_fs_open(c->loop, &uv_open_in_req, c->input_file,
		   O_NONBLOCK | O_RDONLY, 0, NULL)) < 0) {
		errno = -ret;
		perror("uv_fs_open");
		goto exit;
	}

	c->infd = uv_open_in_req.result;
	uv_fs_req_cleanup(&uv_open_in_req);

	DEBUG_LOG("input file open with fd %d", c->infd);

	if ((ret = uv_poll_init(c->loop, c->uv_poll_in_req, c->infd)) < 0 ||
	  (ret = uv_poll_start(c->uv_poll_in_req, UV_READABLE, &on_read)) < 0) {
		errno = -ret;
		perror("uv_poll");
		goto exit;
	}

	STAMP_HANDLE((uv_handle_t*)c->uv_poll_in_req, c);

exit:
	if (ret) {
		uv_fs_t uv_close_req;
		if (c->uv_poll_in_req)
			free(c->uv_poll_in_req);
		if (c->infd) {
			uv_fs_close(c->loop, &uv_close_req, c->infd, NULL);
			uv_fs_req_cleanup(&uv_close_req);
		}
		if (c->uv_poll_in_req->data == c)
			uv_poll_stop(c->uv_poll_in_req);

		/* mark input uninitialized */
		c->uv_poll_in_req = NULL;
	}
	return ret;
}

static int dlparser_init(collector_t* c)
{
	const char* error;

	if ((c->dlparser_handle = dlopen(c->dlparser_path, RTLD_NOW)) == NULL) {
		fprintf(stderr, "dlopen: %s\n", dlerror());
		goto err;
	}
	dlerror(); /* clear any existing error */

	c->parse_parser = dlsym(c->dlparser_handle, "parser_parse");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(parser_parse): %s\n", error);
		goto err;
	}

	c->free_parser = dlsym(c->dlparser_handle, "parser_free");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(parser_free): %s\n", error);
		goto err;
	}

	c->create_parser = dlsym(c->dlparser_handle, "parser_create");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(parser_create): %s\n", error);
		goto err;
	}

	c->reset_parser = dlsym(c->dlparser_handle, "parser_reset");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(parser_reset): %s\n", error);
		goto err;
	}
	return 0;
err:
	errno = EINVAL;
	if (c->dlparser_handle)
		dlclose(c->dlparser_handle);
	return 1;
}

collector_t* collector_create(uv_loop_t* loop, const char* dlparser_path,
  const char* input_file, const char* lua_script)
{
	collector_t* c = NULL;

	if ((c = calloc(1, sizeof(collector_t))) == NULL)
		goto error;

	if (collector_init(c, loop, dlparser_path, input_file, lua_script))
		goto error;

	return c;

error:
	if (c)
		free(c);

	return NULL;
}

int collector_init(collector_t* c, uv_loop_t* loop, const char* dlparser_path,
  const char* input_file, const char* lua_script)
{
	int ret = 0;

	memset(c, 0, sizeof(collector_t));

	c->loop = loop;
	c->input_file = input_file;
	c->lua_script = lua_script;
	c->dlparser_path = dlparser_path;

	if ((c->buf = buf_create(LOGD_BUF_INIT_CAP)) == NULL) {
		perror("buf_create");
		ret = 1;
		goto exit;
	}

	if (c->dlparser_path != NULL && (ret = dlparser_init(c)) != 0) {
		perror("dlparser_init");
		goto exit;
	} else {
		c->parse_parser = (parse_res_t(*)(void*, char*, size_t))(&parser_parse);
		c->free_parser = (void (*)(void*))(&parser_free);
		c->create_parser = (void* (*)())(&parser_create);
		c->reset_parser = (void (*)(void*))(&parser_reset);
	}

	if ((c->parser = c->create_parser()) == NULL) {
		perror("parser_create");
		ret = 1;
		goto exit;
	}

	if ((ret = input_init(c)) != 0) {
		perror("input_init");
		goto exit;
	}

//	if ((ret = collector_load_lua(c))) {
//		perror("collector_load_lua");
//		goto exit;
//	}

exit:
	if (ret)
		collector_deinit(c);

	return ret;
}

int collector_load_lua(collector_t* c)
{
	DEBUG_LOG("collector %p: creating new lua state prev is %p", c, c->lua_state);

	lua_free(c->lua_state);
	if ((c->lua_state = lua_create(c->loop, c->lua_script)) == NULL) {
		perror("lua_create");
		return 1;
	}

	return 0;
}

void collector_deinit(collector_t* c)
{
	uv_fs_t uv_close_req;

	if (c->parser) {
		c->free_parser(c->parser);
		c->parser = NULL;
	}

	if (c->dlparser_handle) {
		dlclose(c->dlparser_handle);
		c->dlparser_handle = NULL;
	}

	if (c->uv_poll_in_req) {
		uv_fs_close(c->loop, &uv_close_req, c->infd, NULL);
		uv_fs_req_cleanup(&uv_close_req);
		uv_poll_stop(c->uv_poll_in_req);
		free(c->uv_poll_in_req);
		c->uv_poll_in_req = NULL;
	}

	if (c->lua_state) {
		lua_free(c->lua_state);
		c->lua_state = NULL;
	}

	if (c->buf) {
		buf_free(c->buf);
		c->buf = NULL;
	}
}

void collector_free(collector_t* c)
{
	if (c) {
		collector_deinit(c);
		free(c);
	}
}
