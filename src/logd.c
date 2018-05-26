#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include "lua.h"
#include "parser.h"
#include "slab/buf.h"
#include "util.h"

#define BUF_CAP 100

// option defaults
#define OPT_DEFAULT_DEBUG false

static struct args_s {
	bool debug;
	const char* input_file;
	int help;
} args;

static char* script;
static parser_t* p;
static lua_t* lstate;
static uv_signal_t sigh;
static uv_loop_t* loop;
static buf_t* b;
static uv_file infd;
static uv_fs_t uv_read_in_req;
static uv_fs_t uv_open_in_req;
static uv_buf_t iov;
static int pret;

void input_close(uv_loop_t* loop, uv_file infd)
{
	uv_fs_t req_in_close;

	uv_fs_close(loop, &req_in_close, infd, NULL);
	uv_fs_req_cleanup(&uv_open_in_req);
	uv_fs_req_cleanup(&uv_read_in_req);
}

void release_all()
{
	input_close(loop, infd);
	lua_free(lstate);
	if (loop) {
		uv_signal_stop(&sigh);
		free(loop);
	}
	buf_free(b);
	parser_free(p);
}

char* args_init(int argc, char* argv[])
{
	/* set defaults for arguments */
	args.debug = OPT_DEFAULT_DEBUG;
	args.input_file = "/dev/stdin";
	args.help = 0;

	static struct option long_options[] = {{"debug", no_argument, 0, 'd'},
	  {"file", required_argument, 0, 'f'}, {"help", no_argument, 0, 'h'},
	  {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	while ((c = getopt_long(
			  argc, argv, "df:o:h", long_options, &option_index)) != -1) {
		switch (c) {
		case 'd':
			args.debug = true;
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

void on_read(uv_fs_t* req)
{
	parse_res_t res;

	if (req->result == 0) {
		DEBUG_LOG("EOF while reading input file %d", infd);
		if (lua_on_eof_defined(lstate))
			lua_call_on_eof(lstate);
		uv_stop(loop);
		return;
	}

	if (req->result < 0) {
		fprintf(stderr, "input read error: %s\n", uv_strerror(req->result));
		uv_stop(loop);
		pret = 1;
		return;
	}

	buf_extend(b, req->result);
	for (;;) {
		res = parser_parse(p, b->next_read, buf_readable(b));
		switch (res.type) {
		case PARSE_PARTIAL:
			if (!buf_full(b)) {
				buf_ack(b, res.consumed);
			} else {
				/* complete log doesn't fit buffer so reserve more space */
				/* do not ack so we re-parse the in the re-allocated buffer */
				if (buf_reserve(b, BUF_CAP) != 0) {
					perror("buf_reserve");
					fprintf(stderr, "error reserving more space in input buffer\n");
					pret = 1;
					return;
				}
				DEBUG_LOG("reserved more space in input buffer: now %zd bytes", b->cap);
				parser_reset(p);
			}
			goto read;
		case PARSE_COMPLETE:
			buf_ack(b, res.consumed);
			lua_call_on_log(lstate, res.result.log);
			parser_reset(p);
			break;
		case PARSE_ERROR:
			DEBUG_LOG("parse error: %s", res.result.error);
			buf_ack(b, res.consumed);
			// TODO call on_error
			parser_reset(p);
			break;
		}
	}

read:
	buf_ecompact(b);
	iov.base = b->next_write;
	iov.len = buf_writable(b);
	uv_fs_read(loop, &uv_read_in_req, infd, &iov, 1, -1, on_read);
}

void on_open(uv_fs_t* req)
{
	infd = uv_open_in_req.result;

	DEBUG_LOG("input file open with fd %d", infd);

	iov.base = b->next_write;
	iov.len = buf_writable(b);
	uv_fs_read(loop, &uv_read_in_req, infd, &iov, 1, -1, on_read);
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

	lua_free(lstate);
	if ((lstate = lua_create(loop, script)) == NULL) {
		perror("lua_create");
		fprintf(stderr, "error reloading script\n");
		exit(1);
	}
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
	printf("\t-d, --debug         enable debug logs [default: %s]\n",
	  OPT_DEFAULT_DEBUG ? "true" : "false");
	printf("\t-f, --file=<path>   file to read data from [default: "
		   "/dev/stdin]\n");
	printf("\t-h, --help          prints this message\n");
}

int main(int argc, char* argv[])
{

	if ((script = args_init(argc, argv)) == NULL || args.help) {
		print_usage(argv[0]);
		goto exit;
	}

	if ((p = parser_create()) == NULL) {
		perror("parser_create");
		pret = 1;
		goto exit;
	}

	if ((b = buf_create(BUF_CAP)) == NULL) {
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

	if ((lstate = lua_create(loop, script)) == NULL) {
		perror("lua_create");
		pret = 1;
		goto exit;
	}

	if ((pret = input_init(loop, args.input_file)) != 0) {
		perror("input_init");
		goto exit;
	}

	if ((pret = signals_init(loop)) != 0)
		goto exit;

	uv_run(loop, UV_RUN_DEFAULT);

exit:
	release_all();
	return pret;
}
