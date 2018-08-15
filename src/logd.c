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
#include "./util.h"

#define OPT_DEFAULT_DEBUG false
#define LINEAL_BACKOFF "lineal"
#define EXP_BACKOFF "exponential"

void on_read_skip(uv_poll_t* req, int status, int events);
void on_read(uv_poll_t* req, int status, int events);

struct args_s {
	int reopen_delay;
	const char* reopen_backoff;
	int reopen_retries;
	char* input_file;
	int help;
	const char* dlparser;
} args;

int pret;
char* script;
void* parser;
void* dlparser_handle;
lua_t* lstate;
buf_t* b;
int curr_reopen_retries;
int backoff;
uv_process_t tail;

uv_signal_t sigusr1, sigusr2, sigint;
uv_loop_t* loop;
uv_file infd;
uv_poll_t uv_poll_in_req;
uv_fs_t uv_open_in_req;

parse_res_t (*parse_parser)(
  void*, char*, size_t) = (parse_res_t(*)(void*, char*, size_t))(&parser_parse);
void (*free_parser)(void*) = (void (*)(void*))(&parser_free);
void* (*create_parser)() = (void* (*)())(&parser_create);
void (*reset_parser)(void*) = (void (*)(void*))(&parser_reset);

void input_try_reopen();
bool try_deferr_input_reopen();

#define LOGD_HANDLE ((void*)"__LOGD_HANDLE")
#define STAMP_HANDLE(handle) (handle)->data = LOGD_HANDLE;
#define RESET_REOPEN_RETRIES()                                                 \
	DEBUG_LOG("reset reopen retries from %d to 0", curr_reopen_retries);       \
	curr_reopen_retries = 0;

void print_version() { printf("logd %s\n", LOGD_VERSION); }

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

	DEBUG_LOG("closing input, fd: %d", infd);

	uv_fs_close(loop, &req_in_close, infd, NULL);
	uv_fs_req_cleanup(&req_in_close);
	uv_poll_stop(&uv_poll_in_req);
	if (tail.pid != 0)
		uv_kill(tail.pid, SIGHUP);
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

void check_input_reopen(int exit_ret)
{
	if (args.reopen_retries != 0) {
		if (curr_reopen_retries != 0) {
			DEBUG_LOG("reopen already scheduled, current retries: %d",
			  curr_reopen_retries);
			input_close();
			if (!try_deferr_input_reopen()) {
				close_logd_uv_handles();
				pret = exit_ret;
			}
		} else {
			input_try_reopen();
		}
	} else {
		close_logd_uv_handles();
		pret = exit_ret;
	}
}

void on_eof()
{
	if (lua_on_eof_defined(lstate)) {
		lua_call_on_eof(lstate);
	}
	check_input_reopen(0);
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

	RESET_REOPEN_RETRIES();

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

	RESET_REOPEN_RETRIES();

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

void on_tail_exit(uv_process_t* req, int64_t exit_status, int term_signal)
{
	DEBUG_LOG(
	  "tail subprocess with pid %d exited with status %ld and signal %d",
	  req->pid, exit_status, term_signal);
	pret = exit_status;

	/* make sure it's not from previous to reopen subprocess */
	if (req->pid != tail.pid) {
		DEBUG_LOG("recv callback of previous tail process, curr_tail_pid: %d",
		  tail.pid);
		return;
	}

	tail.pid = 0;
	check_input_reopen(exit_status ? exit_status : term_signal + 128);
}

int input_init_tail(uv_loop_t* loop, char* input_file)
{
	static uv_process_options_t options;
	static uv_stdio_container_t child_stdio[3];
	static char* args[5];

	args[0] = "tail";
	args[1] = "-n0";
	args[2] = "-f";
	args[3] = input_file;
	args[4] = NULL;

	options.exit_cb = on_tail_exit;
	options.file = "tail";
	options.args = args;
	options.flags = 0;

	child_stdio[0].flags = UV_IGNORE;
	child_stdio[1].flags = UV_INHERIT_FD;
	child_stdio[1].data.fd = 0;
	child_stdio[2].flags = UV_IGNORE;

	options.stdio_count = 3;
	options.stdio = child_stdio;

	int r;
	if ((r = uv_spawn(loop, &tail, &options))) {
		errno = -r;
		perror("uv_spawn");
		return 1;
	}

	DEBUG_LOG("launched pipe subprocess with pid %d..", tail.pid);

	return 0;
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

int input_init(uv_loop_t* loop, char* input_file)
{
	uv_fs_t uv_stat_in_req;
	int ret;
	if ((ret = uv_fs_stat(loop, &uv_stat_in_req, args.input_file, NULL)) < 0) {
		errno = -ret;
		perror("uv_fs_stat");
		return 1;
	}

	int mode = uv_stat_in_req.statbuf.st_mode;
	if (S_ISDIR(mode)) {
		errno = EINVAL;
		perror("args_init: --file, -f");
		return 1;
	}
	uv_fs_req_cleanup(&uv_stat_in_req);

	if (S_ISREG(mode)) {
		if ((ret = input_init_tail(loop, input_file)) != 0 ||
		  (infd = input_open_nonreg(loop, "/dev/stdin")) == -1) {
			return 1;
		}
	} else if ((infd = input_open_nonreg(loop, input_file)) == -1) {
		return 1;
	}

	DEBUG_LOG("input file open, fd: %d", infd);

	if ((ret = uv_poll_init(loop, &uv_poll_in_req, infd)) < 0 ||
	  (ret = uv_poll_start(&uv_poll_in_req, UV_READABLE, &on_read)) < 0) {
		errno = -ret;
		perror("uv_poll");
		return 1;
	}

	STAMP_HANDLE((uv_handle_t*)&uv_poll_in_req);

	return 0;
}

void deferred_input_reopen()
{
	uv_timer_t timer;
	int timeout =
	  next_attempt_backoff(args.reopen_delay, curr_reopen_retries, backoff);

	DEBUG_LOG("reopening pipe in %d milliseconds..", timeout);

	uv_timer_init(loop, &timer);
	uv_timer_start(&timer, input_try_reopen, timeout, 0);
}

bool try_deferr_input_reopen()
{
	if (curr_reopen_retries <= args.reopen_retries) {
		deferred_input_reopen();
		return true;
	}

	return false;
}

void input_try_reopen()
{
	int ret;

	input_close();

	DEBUG_LOG(
	  "trying to reopen file, current retries: %d", curr_reopen_retries);
	curr_reopen_retries += 1;

	if ((ret = input_init(loop, args.input_file)) != 0) {
		perror("input_init");
		if (!try_deferr_input_reopen()) {
			pret = ret;
			close_all_handles();
		}
	}
}

void rel_cfg_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Reloading configuration ...", signum);
	close_lua_uv_handles();
	uv_stop(loop);
}

void rel_log_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Re-opening input files ...", signum);
	input_try_reopen();
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
	  "  -f, --file=<path>		File/Pipe/Socket to read data from [default: "
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
