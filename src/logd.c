#include <dlfcn.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include <slab/buf.h>

#include "./lua.h"
#include "./parser.h"
#include "./util.h"

#define OPT_DEFAULT_DEBUG false

#ifndef LOGD_INIT_BUF_CAP
#define INIT_BUF_CAP 1000000 /* 1MB */
#else
#define INIT_BUF_CAP LOGD_INIT_BUF_CAP
#endif

#ifndef LOGD_BUF_MAX_CAP
#define BUF_MAX_CAP 10000000 /* 10 MB */
#else
#define BUF_MAX_CAP LOGD_BUF_MAX_CAP
#endif

#define SUBMIT_ON_READ(cb)                                                     \
	iov.base = b->next_write;                                                  \
	iov.len = buf_writable(b);                                                 \
	DEBUG_ASSERT(iov.len > 0);                                                 \
	uv_fs_read(loop, &uv_read_in_req, infd, &iov, 1, -1, (cb));

void on_read_skip(uv_fs_t* req);
void on_read(uv_fs_t* req);

static struct args_s {
	bool debug;
	const char* input_file;
	int help;
	const char* dlparser;
} args;

static int pret;
static char* script;
static void* parser;
static void* dlparser_handle;
static lua_t* lstate;
static buf_t* b;

static uv_signal_t sigh;
static uv_loop_t* loop;
static uv_file infd;
static uv_fs_t uv_read_in_req;
static uv_fs_t uv_open_in_req;
static uv_buf_t iov;

static parse_res_t (*parse_parser)(
  void*, char*, size_t) = (parse_res_t(*)(void*, char*, size_t))(&parser_parse);
static void (*free_parser)(void*) = (void (*)(void*))(&parser_free);
static void* (*create_parser)() = (void* (*)())(&parser_create);
static void (*reset_parser)(void*) = (void (*)(void*))(&parser_reset);

void on_close_lua_handle(uv_handle_t* handle)
{
	DEBUG_ASSERT((void*)handle != (void*)&uv_read_in_req);
	DEBUG_ASSERT((void*)handle != (void*)&sigh);
	DEBUG_LOG("closed uv handle %p", handle);
}

void uv_walk_close_lua_handles(uv_handle_t* handle, void* arg)
{
	if ((void*)handle != (void*)&uv_read_in_req &&
	  (void*)handle != (void*)&sigh && !uv_is_closing(handle))
		uv_close(handle, on_close_lua_handle);
}

void close_lua_uv_handles() { uv_walk(loop, uv_walk_close_lua_handles, NULL); }

void close_logd_uv_handles()
{
	uv_fs_t req_in_close;
	uv_fs_close(loop, &req_in_close, infd, NULL);
	uv_fs_req_cleanup(&uv_read_in_req);
	uv_signal_stop(&sigh);
}

void on_eof()
{
	if (lua_on_eof_defined(lstate)) {
		lua_call_on_eof(lstate);
	}
	close_logd_uv_handles();
}

void free_all()
{
	lua_free(lstate);
	if (loop) {
		free(loop);
	}
	free_parser(parser);
	if (dlparser_handle) {
		dlclose(dlparser_handle);
	}
	buf_free(b);
}

char* args_init(int argc, char* argv[])
{
	/* set defaults for arguments */
	args.debug = OPT_DEFAULT_DEBUG;
	args.input_file = "/dev/stdin";
	args.help = 0;
	args.dlparser = NULL;

	static struct option long_options[] = {{"debug", no_argument, 0, 'd'},
	  {"file", required_argument, 0, 'f'}, {"help", no_argument, 0, 'h'},
	  {"parser", required_argument, 0, 'p'}, {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	while ((c = getopt_long(
			  argc, argv, "p:df:o:h", long_options, &option_index)) != -1) {
		switch (c) {
		case 'd':
			args.debug = true;
			break;
		case 'p':
			args.dlparser = optarg;
			break;
		case 'f':
			args.input_file = optarg;
			break;
		case 'h':
			args.help = 1;
			break;
		default:
			abort();
		}
	}

	return argv[optind];
}

int parser_dlload(const char* dlpath)
{
	const char* error;
	DEBUG_LOG("EOF while reading input file %d", infd);

	if ((dlparser_handle = dlopen(dlpath, RTLD_LAZY)) == NULL) {
		fprintf(stderr, "dlopen: %s\n", dlerror());
		goto err;
	}
	dlerror();

	parse_parser = dlsym(dlparser_handle, "parser_parse");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(parser_parse): %s\n", error);
		goto err;
	}

	free_parser = dlsym(dlparser_handle, "parser_free");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(parser_free): %s\n", error);
		goto err;
	}

	create_parser = dlsym(dlparser_handle, "parser_create");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(parser_create): %s\n", error);
		goto err;
	}

	reset_parser = dlsym(dlparser_handle, "parser_reset");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(parser_reset): %s\n", error);
		goto err;
	}
	return 0;
err:
	errno = EINVAL;
	return 1;
}

void on_read_err(uv_fs_t* req)
{
	fprintf(stderr, "input read error: %s\n", uv_strerror(req->result));
	close_lua_uv_handles();
	close_logd_uv_handles();
	pret = 1;
}

void on_read_eof(uv_fs_t* req)
{
	parse_res_t res;
	DEBUG_LOG("EOF while reading input file %d", infd);

parse:
	res = parse_parser(parser, b->next_read, buf_readable(b));
	switch (res.type) {
	case PARSE_COMPLETE:
		buf_ack(b, res.consumed);
		// DEBUG_LOG("parsed new log: %p", &res.log);
		lua_call_on_log(lstate, res.log);
		reset_parser(parser);
		goto parse;
	case PARSE_ERROR:
		DEBUG_LOG("EOF parse error: %s", res.error.msg);
		buf_ack(b, res.consumed);
		if (lua_on_error_defined(lstate)) {
			lua_call_on_error(lstate, res.error.msg, res.log, res.error.at);
		}
		reset_parser(parser);
		goto parse;
	case PARSE_PARTIAL:
		on_eof();
		break;
	}

	return;
}

void on_read_skip(uv_fs_t* req)
{
	parse_res_t res;

	// DEBUG_LOG("result: %zd", req->result);

	if (req->result < 0) {
		on_read_err(req);
		return;
	}

	if (req->result == 0) {
		on_eof();
		return;
	}

	buf_extend(b, req->result);
	res = parse_parser(parser, b->next_read, buf_readable(b));
	if (res.type == PARSE_PARTIAL) {
		buf_reset_offsets(b);
		SUBMIT_ON_READ(&on_read_skip);
		return;
	}

	if (lua_on_error_defined(lstate)) {
		lua_call_on_error(lstate,
		  "log line was skipped because it is more than " STR(
			BUF_MAX_CAP) " bytes",
		  res.log, "");
	}
	buf_ack(b, res.consumed);
	DEBUG_LOG("successfully skipped line: buffer has now %zu readable bytes "
			  "and %zu writable bytes",
	  buf_readable(b), buf_writable(b));
	reset_parser(parser);
	SUBMIT_ON_READ(&on_read);
}

void on_read(uv_fs_t* req)
{
	parse_res_t res;

	// DEBUG_LOG("result: %zd", req->result);

	if (req->result < 0) {
		on_read_err(req);
		return;
	}

	if (req->result == 0) {
		on_read_eof(req);
		return;
	}

	buf_extend(b, req->result);
parse:
	res = parse_parser(parser, b->next_read, buf_readable(b));
	switch (res.type) {

	case PARSE_COMPLETE:
		// DEBUG_LOG("parsed new log: %p", &res.log);
		lua_call_on_log(lstate, res.log);
		buf_consume(b, res.consumed);
		reset_parser(parser);
		goto parse;

	case PARSE_ERROR:
		DEBUG_LOG("parse error: %s", res.error.msg);
		buf_ack(b, res.consumed);
		if (lua_on_error_defined(lstate)) {
			lua_call_on_error(lstate, res.error.msg, res.log, res.error.at);
		}
		reset_parser(parser);
		goto parse;

	case PARSE_PARTIAL:
		if (!buf_full(b)) {
			buf_ack(b, res.consumed);
			goto read;
		}

		if (buf_compact(b)) {
			DEBUG_LOG(
			  "compacted buffer: now writable %zd bytes", buf_writable(b));
			reset_parser(parser);
			goto read;
		}

		/* we have allocated too much memory, start skipping data */
		if (b->cap > BUF_MAX_CAP) {
			DEBUG_LOG("log is too long (more than %d bytes). skipping data...",
			  BUF_MAX_CAP);
			buf_reset_offsets(b);
			goto skip;
		}

		/* complete log doesn't fit buffer so reserve more space */
		/* do not ack so we re-parse with the re-allocated buffer */
		if (buf_reserve(b, INIT_BUF_CAP) != 0) {
			perror("buf_reserve");
			fprintf(stderr, "error reserving more space in input buffer\n");
			pret = 1;
			return;
		}

		DEBUG_LOG("reserved more space in input buffer: now %zd bytes", b->cap);
		reset_parser(parser);
		goto read;
	}

read:
	SUBMIT_ON_READ(&on_read);
	return;
skip:
	SUBMIT_ON_READ(&on_read_skip);
	return;
}

void on_open(uv_fs_t* req)
{
	infd = uv_open_in_req.result;
	uv_fs_req_cleanup(&uv_open_in_req);
	DEBUG_LOG("input file open with fd %d", infd);
	SUBMIT_ON_READ(&on_read);
}

int input_init(uv_loop_t* loop, const char* input_file)
{
	int ret;

	if ((ret = uv_fs_open(
		   loop, &uv_open_in_req, input_file, O_RDONLY, 0, on_open)) < 0) {
		errno = -ret;
		perror("uv_fs_open");
		return 1;
	}

	return 0;
}

void sigusr1_signal_handler(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("received signal %d", signum);
	close_lua_uv_handles();
	uv_stop(loop);
}

int signals_init(uv_loop_t* loop)
{
	int ret;

	if ((ret = uv_signal_init(loop, &sigh)) < 0)
		goto error;

	if ((ret = uv_signal_start(&sigh, sigusr1_signal_handler, SIGUSR1)) < 0)
		goto error;

	DEBUG_LOG("initialized SIGUSR1 signal handler %p", &sigh);
	return 0;
error:
	fprintf(
	  stderr, "uv_signal_init: %s: %s\n", uv_err_name(ret), uv_strerror(ret));
	return ret;
}

int loop_create()
{
	int ret;

	if ((loop = calloc(1, sizeof(uv_loop_t))) == NULL) {
		errno = ENOMEM;
		goto error;
	}

	if ((ret = uv_loop_init(loop)) < 0) {
		fprintf(stderr, "%s: %s\n", uv_err_name(ret), uv_strerror(ret));
		goto error;
	}

	DEBUG_LOG("initialized event loop %p", loop);
	return 0;

error:
	if (loop) {
		free(loop);
		loop = NULL;
	}
	return 1;
}

void print_usage(const char* exe)
{
	printf("usage: %s <script> [options]\n", exe);
	printf("\noptions:\n");
	printf("\t-d, --debug			enable debug logs [default: %s]\n",
	  OPT_DEFAULT_DEBUG ? "true" : "false");
	printf("\t-f, --file=<path>		file to read data from [default: "
		   "/dev/stdin]\n");
	printf("\t-p, --parser=<parser_so>	load dynamic shared object C ABI "
		   "parser via dlopen [default: "
		   "builtin]\n");
	printf("\t-h, --help			prints this message\n");
}

int main(int argc, char* argv[])
{

	if ((script = args_init(argc, argv)) == NULL || args.help) {
		print_usage(argv[0]);
		goto exit;
	}

	if (args.dlparser != NULL && (pret = parser_dlload(args.dlparser)) != 0) {
		perror("parser_dlload");
		goto exit;
	}

	if ((parser = create_parser()) == NULL) {
		perror("parser_create");
		pret = 1;
		goto exit;
	}

	if ((b = buf_create(INIT_BUF_CAP)) == NULL) {
		perror("buf_create");
		pret = 1;
		goto exit;
	}

	if ((pret = loop_create()) != 0) {
		perror("loop_create");
		goto exit;
	}

	if ((pret = uv_loop_init(loop)) < 0) {
		fprintf(stderr, "%s: %s\n", uv_err_name(pret), uv_strerror(pret));
		goto exit;
	}

	if ((pret = input_init(loop, args.input_file)) != 0) {
		perror("input_init");
		goto exit;
	}

	if ((pret = signals_init(loop)) != 0)
		goto exit;

	/* sigusr1 signal handler calls uv_stop but leaves non-lua uv handles open
	 */
	do {
		DEBUG_LOG("creating new lua state prev is %p", lstate);

		lua_free(lstate);
		if ((lstate = lua_create(loop, script)) == NULL) {
			perror("lua_create");
			pret = 1;
			goto exit;
		}

	} while (uv_run(loop, UV_RUN_DEFAULT));

	DEBUG_ASSERT(uv_loop_alive(loop) == 0);

exit:
	free_all();
	return pret;
}
