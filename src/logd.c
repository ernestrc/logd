#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include "lua.h"
#include "parser.h"
#include "slab/buf.h"
#include "util.h"

#define BUF_CAP 100000

// option defaults
#define OPT_DEFAULT_DEBUG false

static struct args_s {
	bool debug;
	const char* input_file;
	int help;
} args;

static parser_t* p;
static lua_t* l;
static buf_t* b;
static uv_file infd;
static uv_fs_t uv_read_in_req;
static uv_fs_t uv_open_in_req;
static uv_buf_t iov;

void input_close(uv_loop_t* loop, uv_file infd)
{
	uv_fs_t req_in_close;

	uv_fs_close(loop, &req_in_close, infd, NULL);
	uv_fs_req_cleanup(&uv_open_in_req);
	uv_fs_req_cleanup(&uv_read_in_req);
}

void release_all()
{
	if (l) {
		input_close(l->loop, infd);
		uv_stop(l->loop);
	}
	buf_free(b);
	parser_free(p);
	lua_free(l);
}

int args_init(int argc, char* argv[], char** script)
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

	*script = argv[optind];

	return 0;
}

void on_read(uv_fs_t* req)
{
	presult_t res;

	if (req->result > 0) {
		buf_extend(b, req->result);
		res = (presult_t){false, 0};
		for (;;) {
			res = parser_parse(p, b->next_read, buf_readable(b));
			buf_consume(b, res.consumed);
			if (!res.complete) {
				break;
			}
#ifndef LOGD_DEBUG
			lua_call_on_log(l, &p->result);
#else
			if (lua_pcall_on_log(l, &p->result) != 0) {
				fprintf(stderr, "on_log runtime error: %s\n",
				  uv_strerror(req->result));
				release_all();
				exit(1);
			}
#endif
		}
		iov.base = b->next_write;
		iov.len = buf_writable(b);
		uv_fs_read(l->loop, &uv_read_in_req, infd, &iov, 1, -1, on_read);
	} else if (req->result < 0) {
		fprintf(stderr, "input read error: %s\n", uv_strerror(req->result));
		release_all();
		exit(1);
	}
}

int input_init(uv_loop_t* loop, const char* input_file)
{
	int ret;

	if ((ret = uv_fs_open(
		   loop, &uv_open_in_req, input_file, O_RDONLY, 0, NULL)) < 0) {
		errno = -ret;
		perror("uv_fs_open");
		return 1;
	}

	infd = uv_open_in_req.result;
	iov.base = b->next_write;
	iov.len = buf_writable(b);
	uv_fs_read(l->loop, &uv_read_in_req, infd, &iov, 1, -1, on_read);

	return 0;
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
	char* script;
	int ret = 0;

	if ((ret = args_init(argc, argv, &script)) != 0 || script == NULL ||
	  args.help) {
		print_usage(argv[0]);
		goto exit;
	}

	if ((p = parser_create()) == NULL) {
		perror("parser_create");
		ret = 1;
		goto exit;
	}

	if ((b = buf_create(BUF_CAP)) == NULL) {
		perror("buf_create");
		ret = 1;
		goto exit;
	}

	if ((l = lua_create(script)) == NULL) {
		perror("lua_create");
		ret = 1;
		goto exit;
	}

	if ((ret = input_init(l->loop, args.input_file)) != 0) {
		perror("input_init");
		goto exit;
	}

	ret = uv_run(l->loop, UV_RUN_DEFAULT);

exit:
	release_all();
	return ret;
}
