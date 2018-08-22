#include <dlfcn.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <slab/buf.h>

#include "./config.h"
#include "./lua.h"
#include "./parser.h"
#include "./tail.h"
#include "./util.h"

#define OPT_DEFAULT_DEBUG false
#define LINEAL_BACKOFF "lineal"
#define EXP_BACKOFF "exponential"

#define LOGD_HANDLE ((void*)"__LOGD_HANDLE")
#define STAMP_HANDLE(handle) (handle)->data = LOGD_HANDLE;
#define STDIN_INPUT_FILE "/dev/stdin"

struct args_s {
	int reopen_delay;
	const char* reopen_backoff;
	int reopen_retries;
	char* input_file;
	int help;
	const char* dlparser;
} args;

enum input_state_e {
	CLOSED_ISTATE,
	OPENING_ISTATE,
	READING_ISTATE,
	EXIT_ISTATE,
} input_state;

int pret;
char* script;
char* log_start;
void* parser;
void* dlparser_handle;
lua_t* lstate;
buf_t* b;
uv_file infd;
tail_t* tail;
uv_poll_t uv_poll_in_req;
uv_loop_t* loop;
int curr_reopen_retries;
int backoff;
int input_is_reg;
uv_signal_t sigusr1, sigusr2, sigint;
uv_fs_t uv_open_in_req;

parse_res_t (*parse_parser)(
  void*, char*, size_t) = (parse_res_t(*)(void*, char*, size_t))(&parser_parse);
void (*free_parser)(void*) = (void (*)(void*))(&parser_free);
void* (*create_parser)() = (void* (*)())(&parser_create);
void (*reset_parser)(void*) = (void (*)(void*))(&parser_reset);

void on_read_skip(uv_poll_t* req, int status, int events);
void on_read(uv_poll_t* req, int status, int events);
void on_first_read(uv_poll_t* req, int status, int events);
void input_close();
static void input_reopen_open();
int input_open(uv_loop_t*, tail_t*, char*);

void print_version() { printf("logd %s\n", LOGD_VERSION); }

void print_usage(const char* exe)
{
	printf("usage: %s <script> [options]\n", exe);
	printf("\noptions:\n");
	printf(
	  "  -f, --file=<path>		File to ingest appended data from [default: "
	  "/dev/stdin]\n");
	printf("  -p, --parser=<parser_so>	Load shared object "
		   "parser via dlopen [default: "
		   "builtin]\n");
	printf("  -r, --reopen-retries		Reopen retries on EOF before giving up "
		   "[default: %d]\n",
	  args.reopen_retries);
	printf("  -d, --reopen-delay		Retry reopen delay in milliseconds "
		   "[default: %d]\n",
	  args.reopen_delay);
	printf("  -b, --reopen-backoff		Retry reopen delay backoff 'linear' or "
		   "'exponential'"
		   "[default: %s]\n",
	  args.reopen_backoff);
	printf("  -h, --help			Display this message.\n");
	printf("  -v, --version			Display logd version information.\n");
}

char* args_init(int argc, char* argv[])
{
	/* set defaults for arguments */
	args.input_file = STDIN_INPUT_FILE;
	args.help = 0;
	args.dlparser = NULL;
	args.reopen_backoff = LINEAL_BACKOFF;
	args.reopen_retries = 0;
	args.reopen_delay = 100; /* milliseconds */

	static struct option long_options[] = {
	  {"reopen-backoff", required_argument, 0, 'b'},
	  {"reopen-retries", required_argument, 0, 'r'},
	  {"reopen-delay", required_argument, 0, 'd'},
	  {"file", required_argument, 0, 'f'}, {"help", no_argument, 0, 'h'},
	  {"parser", required_argument, 0, 'p'}, {"version", no_argument, 0, 'v'},
	  {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	while ((c = getopt_long(
			  argc, argv, "vp:f:hb:d:r:", long_options, &option_index)) != -1) {
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
		case 'r':
			if ((args.reopen_retries = parse_non_negative_int(optarg)) == -1) {
				perror("parse --reopen-retries");
				return NULL;
			}
			break;
		case 'b':
			args.reopen_backoff = optarg;
			if (strncmp(LINEAL_BACKOFF, optarg, strlen(LINEAL_BACKOFF)) != 0 &&
			  strncmp(EXP_BACKOFF, optarg, strlen(EXP_BACKOFF)) != 0) {
				errno = EINVAL;
				perror("--reopen-backoff");
				return NULL;
			}
			backoff =
			  strncmp(LINEAL_BACKOFF, optarg, strlen(LINEAL_BACKOFF)) == 0 ? 1 :
																			 2;
			break;
		case 'd':
			if ((args.reopen_delay = parse_non_negative_int(optarg)) == -1 ||
			  args.reopen_delay == 0) {
				perror("parse --reopen-delay");
				return NULL;
			}
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

void on_close_lua_handle(uv_handle_t* handle)
{
	DEBUG_LOG("closed uv handle %p, handles: %d", handle, loop->active_handles);
}

void uv_walk_close_lua_handles(uv_handle_t* handle, void* arg)
{
	if (handle->data == LOGD_HANDLE || handle->data == tail ||
	  uv_is_closing(handle)) {
		DEBUG_LOG("skipping uv handle %p", handle);
		return;
	}

	DEBUG_LOG("closing uv handle %p", handle);
	uv_close(handle, on_close_lua_handle);
}

void close_lua_uv_handles() { uv_walk(loop, uv_walk_close_lua_handles, NULL); }

void close_logd_uv_handles()
{
	DEBUG_LOG("closing logd libuv handles, handles: %d", loop->active_handles);

	input_close();
	uv_signal_stop(&sigusr1);
	uv_signal_stop(&sigusr2);
	uv_signal_stop(&sigint);
}

void close_all(int exit_status)
{
	DEBUG_LOG("closing all libuv handles, handles: %d", loop->active_handles);

	close_lua_uv_handles();
	close_logd_uv_handles();

	pret = exit_status;
	input_state = EXIT_ISTATE;
}

// NOTE: only used one at a time during input reopening mechanism
static void set_timeout(int timeout, void (*func)(uv_timer_t*))
{
	static uv_timer_t timer;

	uv_timer_init(loop, &timer);
	uv_timer_start(&timer, func, timeout, 0);
}

static void input_reopen()
{
	DEBUG_LOG(
	  "trying to reopen file, current retries: %d", curr_reopen_retries);

	input_close();
	input_reopen_open();
}

static void input_reopen_open()
{
	int ret;

	curr_reopen_retries += 1;
	DEBUG_LOG("reopen attempt, state: %d, current_try: %d", input_state,
	  curr_reopen_retries);

	if ((ret = input_open(loop, tail, args.input_file)) != 0) {
		perror("input_open");
		if (curr_reopen_retries <= args.reopen_retries) {
			set_timeout(next_attempt_backoff(
						  args.reopen_delay, curr_reopen_retries, backoff),
			  input_reopen);
		} else {
			close_logd_uv_handles();
		}
	}
}

void input_close()
{
	uv_fs_t req_in_close;

	DEBUG_LOG("input close attempt, state: %d", input_state);

	switch (input_state) {
	case EXIT_ISTATE:
	case CLOSED_ISTATE:
		return;
	case OPENING_ISTATE:
		/* fallthrough */
	case READING_ISTATE:
		break;
	}

	DEBUG_LOG("closing input, fd: %d", infd);

	if (input_is_reg) {
		tail_close(tail);
	} else {
		uv_fs_close(loop, &req_in_close, infd, NULL);
		uv_fs_req_cleanup(&req_in_close);
	}

	uv_poll_stop(&uv_poll_in_req);

	input_state = CLOSED_ISTATE;
}

void input_reopen_attempt(int exit_status)
{
	void (*open_func)(uv_timer_t*);

	DEBUG_LOG("input reopen attempt, exit_status: %d, state: %d", exit_status,
	  input_state);

	switch (input_state) {
	case OPENING_ISTATE:
		DEBUG_LOG("reopen mechanism in-progress, current_retry: %d",
		  curr_reopen_retries);
		/* falltrhough */
	case EXIT_ISTATE:
		return;
	case CLOSED_ISTATE:
		open_func = input_reopen_open;
		break;
	case READING_ISTATE:
		open_func = input_reopen;
		break;
	}

	if (args.reopen_retries != 0) {
		input_state = OPENING_ISTATE;
		set_timeout(
		  next_attempt_backoff(args.reopen_delay, curr_reopen_retries, backoff),
		  open_func);
	} else {
		close_logd_uv_handles();
	}
}

int input_open_nonreg(uv_loop_t* loop, char* input_file)
{
	int ret;

	if ((ret = uv_fs_open(loop, &uv_open_in_req, input_file,
		   O_NONBLOCK | O_RDONLY, 0, NULL)) < 0) {
		errno = -ret;
		perror("uv_fs_open");
		ret = -1;
		goto exit;
	}

	ret = uv_open_in_req.result;

exit:
	uv_fs_req_cleanup(&uv_open_in_req);
	return ret;
}

int input_open(uv_loop_t* loop, tail_t* tail, char* input_file)
{
	uv_fs_t uv_stat_in_req;
	int ret, mode;

	DEBUG_LOG("input open, input_file: %s, state: %d", input_file, input_state);

	switch (input_state) {
	case EXIT_ISTATE:
		/* fallthrough */
	case READING_ISTATE:
		return 0;
	case OPENING_ISTATE: // only reached when called from reopen mechanism
		/* fallthrough */
	case CLOSED_ISTATE:
		break;
	}

	if ((ret = uv_fs_stat(loop, &uv_stat_in_req, input_file, NULL)) < 0) {
		errno = -ret;
		perror("uv_fs_stat");
		return 1;
	}

	mode = uv_stat_in_req.statbuf.st_mode;
	if (S_ISDIR(mode)) {
		errno = EINVAL;
		perror("args_init: --file, -f");
		return 1;
	}
	uv_fs_req_cleanup(&uv_stat_in_req);

	if ((input_is_reg = S_ISREG(mode))) {
		if ((infd = tail_open(tail, input_reopen_attempt)) < 0) {
			perror("tail_open");
			return 1;
		}
	} else if ((infd = input_open_nonreg(loop, input_file)) < 0) {
		perror("input_open_nonreg");
		return 1;
	}

	if ((ret = uv_poll_init(loop, &uv_poll_in_req, infd)) ||
	  (ret = uv_poll_start(&uv_poll_in_req, UV_READABLE, &on_read)) < 0) {
		errno = -ret;
		perror("uv_poll");
		return 1;
	}

	STAMP_HANDLE((uv_handle_t*)&uv_poll_in_req);
	input_state = OPENING_ISTATE;

	return 0;
}

int logd_buf_reserve()
{
	int ret;
	int offset = log_start - b->buf;
	ret = buf_reserve(b, LOGD_BUF_INIT_CAP);
	reset_parser(parser);
	log_start = b->buf + offset;

	return ret;
}

void logd_reset_parser()
{
	reset_parser(parser);
	log_start = b->next_read;
}

bool logd_buf_compact()
{
	if (b->buf == log_start)
		return 0;

	int moved = log_start - b->buf;
	int len = b->last - log_start;
	log_start = memmove(b->buf, log_start, len);
	b->next_write = b->buf + len;
	b->next_read = b->next_read - moved;

	return 1;
}

void on_eof()
{
	if (lua_on_eof_defined(lstate)) {
		lua_call_on_eof(lstate);
	}
	input_reopen_attempt(0);
}

void on_poll_err(int ret)
{
	errno = -ret;
	perror("uv_poll");
	close_all(1);
}

void on_read_err()
{
	perror("input read");
	close_all(1);
}

void on_read_eof()
{
	parse_res_t res;
	DEBUG_LOG("EOF while reading input file %d", infd);

parse:
	res = parse_parser(parser, b->next_read, buf_readable(b));
	switch (res.type) {
	case PARSE_COMPLETE:
		res.log->is_safe = true;
		lua_call_on_log(lstate, res.log);
		buf_ack(b, res.consumed);
		logd_reset_parser();
		res.log->is_safe = false;
		goto parse;
	case PARSE_ERROR:
		DEBUG_LOG("EOF parse error: %s", res.error.msg);
		buf_ack(b, res.consumed);
		if (lua_on_error_defined(lstate)) {
			lua_call_on_error(lstate, res.error.msg, res.log, res.error.at);
		}
		logd_reset_parser();
		goto parse;
	case PARSE_PARTIAL:
		on_eof();
		break;
	}

	return;
}

#define READ(req, status, read_len, on_eof_h)                                  \
	if (status < 0) {                                                          \
		on_poll_err(status);                                                   \
		return;                                                                \
	}                                                                          \
                                                                               \
	errno = 0;                                                                 \
	int ret = read(infd, b->next_write, read_len);                             \
	if (errno == EAGAIN) {                                                     \
		DEBUG_LOG("input not ready: %d", infd);                                \
		return;                                                                \
	}                                                                          \
	if (ret < 0) {                                                             \
		on_read_err();                                                         \
		return;                                                                \
	}                                                                          \
                                                                               \
	if (ret == 0 && read_len != 0) {                                           \
		on_eof_h();                                                            \
		return;                                                                \
	}                                                                          \
	buf_extend(b, ret);

void on_read_skip(uv_poll_t* req, int status, int events)
{
	parse_res_t res;

	READ(req, status, buf_writable(b), on_eof);

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
	logd_reset_parser();

	if ((ret = uv_poll_stop(&uv_poll_in_req)) < 0 ||
	  (ret = uv_poll_start(&uv_poll_in_req, UV_READABLE, &on_read)) < 0) {
		on_poll_err(ret);
	}
}

void on_read(uv_poll_t* req, int status, int events)
{
	parse_res_t res;

	READ(req, status, buf_writable(b), on_read_eof);

	// TODO optimize
	curr_reopen_retries = 0;
	input_state = READING_ISTATE;

parse:
	res = parse_parser(parser, b->next_read, buf_readable(b));
	switch (res.type) {

	case PARSE_COMPLETE:
		// DEBUG_LOG("parsed new log: %p", &res.log);
		res.log->is_safe = true;
		lua_call_on_log(lstate, res.log);
		buf_consume(b, res.consumed);
		logd_reset_parser();
		res.log->is_safe = false;
		goto parse;

	case PARSE_ERROR:
		DEBUG_LOG("parse error: %s", res.error.msg);
		buf_ack(b, res.consumed);
		if (lua_on_error_defined(lstate)) {
			lua_call_on_error(lstate, res.error.msg, res.log, res.error.at);
		}
		logd_reset_parser();
		goto parse;

	case PARSE_PARTIAL:
		if (!buf_full(b)) {
			buf_ack(b, res.consumed);
			goto read;
		}

		/* compact if we have data from previous log in buffer */
		if (logd_buf_compact()) {
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
		if (logd_buf_reserve() != 0) {
			perror("buf_reserve");
			fprintf(stderr, "error reserving more space in input buffer\n");
			pret = 1;
			return;
		}

		DEBUG_LOG("reserved more space in input buffer: now %zd bytes", b->cap);
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

void rel_cfg_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Reloading lua script ...", signum);
	close_lua_uv_handles();
	uv_stop(loop);
}

void rel_log_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Re-opening input files ...", signum);
	input_state = OPENING_ISTATE;
	input_reopen();
}

void shutdown_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Shutdown ...", signum);
	close_all(signum + 128);
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

void free_all()
{
	lua_free(lstate);
	tail_free(tail);
	buf_free(b);
	free_parser(parser);
	if (loop) {
		uv_loop_close(loop);
		free(loop);
	}
	if (dlparser_handle) {
		dlclose(dlparser_handle);
	}
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

	logd_reset_parser();

	if ((pret = loop_create()) != 0) {
		perror("loop_create");
		goto exit;
	}

	if ((pret = uv_loop_init(loop)) < 0) {
		fprintf(stderr, "%s: %s\n", uv_err_name(pret), uv_strerror(pret));
		goto exit;
	}

	if ((tail = tail_create(loop, args.input_file)) == NULL) {
		perror("tail_create");
		goto exit;
	}

	if ((pret = input_open(loop, tail, args.input_file)) != 0) {
		perror("input_open");
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
