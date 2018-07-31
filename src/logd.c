#include <dlfcn.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <slab/buf.h>

#include "./config.h"
#include "./lua.h"
#include "./parser.h"
#include "./util.h"

#define OPT_DEFAULT_DEBUG false

void on_read_skip(uv_poll_t* req, int status, int events);
void on_read(uv_poll_t* req, int status, int events);

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

static uv_signal_t sigusr1, sigusr2, sigint;
static uv_loop_t* loop;
static uv_file infd;
static uv_poll_t uv_poll_in_req;

static parse_res_t (*parse_parser)(
  void*, char*, size_t) = (parse_res_t(*)(void*, char*, size_t))(&parser_parse);
static void (*free_parser)(void*) = (void (*)(void*))(&parser_free);
static void* (*create_parser)() = (void* (*)())(&parser_create);
static void (*reset_parser)(void*) = (void (*)(void*))(&parser_reset);

void print_version() { printf("logd %s\n", LOGD_VERSION); }

#define LOGD_HANDLE ((void*)"__LOGD_HANDLE")
#define STAMP_HANDLE(handle) (handle)->data = LOGD_HANDLE;

void on_close_lua_handle(uv_handle_t* handle)
{
	DEBUG_LOG("closed uv handle %p", handle);
}

void uv_walk_close_lua_handles(uv_handle_t* handle, void* arg)
{
	if (handle->data == LOGD_HANDLE || uv_is_closing(handle)) {
		DEBUG_LOG("skipping uv handle %p", handle);
		return;
	}

	DEBUG_LOG("closing uv handle %p", handle);
	uv_close(handle, on_close_lua_handle);
}

void close_lua_uv_handles() { uv_walk(loop, uv_walk_close_lua_handles, NULL); }

void input_close()
{
	uv_fs_t req_in_close;
	uv_fs_close(loop, &req_in_close, infd, NULL);
	uv_fs_req_cleanup(&req_in_close);
	uv_poll_stop(&uv_poll_in_req);
}

void close_logd_uv_handles()
{
	input_close();
	uv_signal_stop(&sigusr1);
	uv_signal_stop(&sigusr2);
	uv_signal_stop(&sigint);
}

void close_all_handles()
{
	close_lua_uv_handles();
	close_logd_uv_handles();
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
		uv_loop_close(loop);
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
	args.input_file = "/dev/stdin";
	args.help = 0;
	args.dlparser = NULL;

	static struct option long_options[] = {{"debug", no_argument, 0, 'd'},
	  {"file", required_argument, 0, 'f'}, {"help", no_argument, 0, 'h'},
	  {"parser", required_argument, 0, 'p'}, {"version", no_argument, 0, 'v'},
	  {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	while ((c = getopt_long(
			  argc, argv, "vp:df:o:h", long_options, &option_index)) != -1) {
		switch (c) {
		case 'v':
			print_version();
			exit(0);
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

	/* check that -f/--file arg is not a directory or a regular file */
	uv_fs_t uv_stat_in_req;
	int ret;
	if ((ret = uv_fs_stat(loop, &uv_stat_in_req, args.input_file, NULL)) < 0) {
		errno = -ret;
		perror("uv_fs_stat");
		return NULL;
	}

	int mode = uv_stat_in_req.statbuf.st_mode;
	if (S_ISREG(mode) || S_ISDIR(mode)) {
		errno = EINVAL;
		perror("args_init: --file, -f");
		return NULL;
	}

	uv_fs_req_cleanup(&uv_stat_in_req);

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

void on_poll_err(int ret)
{
	errno = -ret;
	perror("uv_poll");
	close_all_handles();
	pret = 1;
}

void on_read_err()
{
	perror("input read");
	close_all_handles();
	pret = 1;
}

void on_read_eof()
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

void on_read_skip(uv_poll_t* req, int status, int events)
{
	parse_res_t res;
	if (status < 0) {
		on_poll_err(status);
		return;
	}

	errno = 0;
	int ret = read(infd, b->next_write, buf_writable(b));
	if (errno == EAGAIN) {
		DEBUG_LOG("input not ready: %d", infd);
		return;
	}
	if (ret < 0) {
		on_read_err();
		return;
	}

	if (ret == 0) {
		on_eof();
		return;
	}

	buf_extend(b, ret);
	res = parse_parser(parser, b->next_read, buf_readable(b));
	if (res.type == PARSE_PARTIAL) {
		buf_reset_offsets(b);
		return;
	}

	if (lua_on_error_defined(lstate)) {
		lua_call_on_error(lstate,
		  "log line was skipped because it is more than " STR(
			LOGD_BUF_MAX_CAP) " bytes",
		  res.log, "");
	}
	buf_ack(b, res.consumed);
	DEBUG_LOG("successfully skipped line: buffer has now %zu readable bytes "
			  "and %zu writable bytes",
	  buf_readable(b), buf_writable(b));
	reset_parser(parser);

	if ((ret = uv_poll_stop(&uv_poll_in_req)) < 0 ||
	  (ret = uv_poll_start(&uv_poll_in_req, UV_READABLE, &on_read)) < 0) {
		on_poll_err(ret);
	}
}

void on_read(uv_poll_t* req, int status, int events)
{
	parse_res_t res;
	if (status < 0) {
		on_poll_err(status);
		return;
	}

	errno = 0;
	int ret = read(infd, b->next_write, buf_writable(b));
	if (errno == EAGAIN) {
		DEBUG_LOG("input not ready: %d", infd);
		return;
	}
	if (ret < 0) {
		on_read_err();
		return;
	}

	if (ret == 0) {
		on_read_eof();
		return;
	}

	buf_extend(b, ret);
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
		if (b->cap > LOGD_BUF_MAX_CAP) {
			DEBUG_LOG("log is too long (more than %d bytes). skipping data...",
			  LOGD_BUF_MAX_CAP);
			buf_reset_offsets(b);
			goto skip;
		}

		/* complete log doesn't fit buffer so reserve more space */
		/* do not ack so we re-parse with the re-allocated buffer */
		if (buf_reserve(b, LOGD_BUF_INIT_CAP) != 0) {
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
	return;
skip:
	if ((ret = uv_poll_stop(&uv_poll_in_req)) < 0 ||
	  (ret = uv_poll_start(&uv_poll_in_req, UV_READABLE, &on_read_skip)) < 0) {
		on_poll_err(ret);
	}
	return;
}

int input_init(uv_loop_t* loop, const char* input_file)
{
	int ret;
	uv_fs_t uv_open_in_req;

	if ((ret = uv_fs_open(loop, &uv_open_in_req, input_file,
		   O_NONBLOCK | O_RDONLY, 0, NULL)) < 0) {
		errno = -ret;
		perror("uv_fs_open");
		goto exit;
	}

	infd = uv_open_in_req.result;
	uv_fs_req_cleanup(&uv_open_in_req);

	DEBUG_LOG("input file open with fd %d", infd);

	if ((ret = uv_poll_init(loop, &uv_poll_in_req, infd)) < 0 ||
	  (ret = uv_poll_start(&uv_poll_in_req, UV_READABLE, &on_read)) < 0) {
		errno = -ret;
		perror("uv_poll");
		goto exit;
	}

	STAMP_HANDLE((uv_handle_t*)&uv_poll_in_req);

exit:
	return ret;
}

void rel_cfg_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Reloading configuration ...", signum);
	close_lua_uv_handles();
	uv_stop(loop);
}

void rel_log_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Re-opening log files ...", signum);
	input_close();
	if ((pret = input_init(loop, args.input_file)) != 0) {
		perror("input_init");
		close_all_handles();
	}
}

void shutdown_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Shutdown ...", signum);
	close_all_handles();
	pret = signum + 128;
}

int signals_init(uv_loop_t* loop)
{
	int ret;

	if ((ret = uv_signal_init(loop, &sigusr1)) < 0)
		goto error;

	if ((ret = uv_signal_init(loop, &sigusr2)) < 0)
		goto error;

	if ((ret = uv_signal_init(loop, &sigint)) < 0)
		goto error;

	if ((ret = uv_signal_start(&sigusr1, rel_cfg_sig_h, SIGUSR1)) < 0)
		goto error;

	if ((ret = uv_signal_start(&sigusr2, rel_log_sig_h, SIGUSR2)) < 0)
		goto error;

	if ((ret = uv_signal_start(&sigint, shutdown_sig_h, SIGINT)) < 0)
		goto error;

	STAMP_HANDLE((uv_handle_t*)&sigusr1);
	STAMP_HANDLE((uv_handle_t*)&sigusr2);
	STAMP_HANDLE((uv_handle_t*)&sigint);

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
	printf(
	  "\t-f, --file=<path>		Pipe/Unix Socket to read data from [default: "
	  "/dev/stdin]\n");
	printf("\t-p, --parser=<parser_so>	Load shared object "
		   "parser via dlopen [default: "
		   "builtin]\n");
	printf("\t-h, --help			Display this message.\n");
	printf("\t-v, --version			Display logd version information.\n");
}

int main(int argc, char* argv[])
{

	if ((script = args_init(argc, argv)) == NULL || args.help) {
		print_usage(argv[0]);
		pret = !args.help;
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

	if ((b = buf_create(LOGD_BUF_INIT_CAP)) == NULL) {
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
	DEBUG_LOG("realeased all resources; exiting with status code: %d", pret);
	return pret;
}
