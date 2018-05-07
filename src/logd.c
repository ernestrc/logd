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

struct args_s {
	bool debug;
	const char* input_file;
	const char* output_file;
	int help;
} args;

parser_t* p;
lua_t* l;
buf_t* b;

uv_file infd;
uv_file outfd;
uv_fs_t uv_read_in_req;
// uv_fs_t uv_read_out_req;
uv_fs_t uv_open_in_req;
// uv_fs_t uv_open_out_req;
uv_buf_t iov;

// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
// TODO do it well now!
int args_init(int argc, char* argv[], char** script)
{
	/* set defaults for arguments */
	args.debug = OPT_DEFAULT_DEBUG;
	args.input_file = "/dev/stdin";
	// args.output_file = "/dev/stderr";
	args.help = 0;

	static struct option long_options[] = {{"debug", no_argument, 0, 'd'},
	  {"file", required_argument, 0, 'f'}, {"out", required_argument, 0, 'o'},
	  {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	int fd;
	while ((c = getopt_long(
			  argc, argv, "df:o:h", long_options, &option_index)) != -1) {
		switch (c) {
		case 'd':
			args.debug = true;
			break;
		case 'f':
			args.input_file = optarg;
			break;
		case 'o':
			args.output_file = optarg;
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

void files_close(uv_loop_t* loop, uv_file infd, uv_file outfd)
{
	uv_fs_t req_in_close;
	// uv_fs_t req_out_close;

	uv_fs_close(loop, &req_in_close, infd, NULL);
	// uv_fs_close(loop, &req_out_close, outfd, NULL);

	uv_fs_req_cleanup(&uv_open_in_req);
	uv_fs_req_cleanup(&uv_read_in_req);
	// uv_fs_req_cleanup(&uv_open_out_req);
	// uv_fs_req_cleanup(&uv_read_out_req);
}

void on_read(uv_fs_t* req)
{
	presult_t res;

	if (req->result < 0) {
		fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
	} else if (req->result == 0) {
		/* done */
	} else if (req->result > 0) {
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
			if (lua_pcall_on_log(l, &p->result) != 0)
				return 1;
#endif
		}
		iov.base = b->next_write;
		iov.len = buf_writable(b);
		uv_fs_read(l->loop, &uv_read_in_req, infd, &iov, 1, -1, on_read);
	}
}

int files_init(uv_loop_t* loop, const char* input_file, const char* output_file)
{
	int ret;

	if ((ret = uv_fs_open(
		   loop, &uv_open_in_req, input_file, O_RDONLY, 0, NULL)) < 0) {
		errno = -ret;
		perror("uv_fs_open");
		return 1;
	}

	// if ((ret = uv_fs_open(
	// 	   loop, &out_uv, output_file, O_CREAT | O_WRONLY, 0, NULL)) < 0) {
	// 	errno = -ret;
	// 	perror("uv_fs_open");
	// 	return 1;
	// }

	infd = uv_open_in_req.result;
	iov.base = b->next_write;
	iov.len = buf_writable(b);
	uv_fs_read(l->loop, &uv_read_in_req, infd, &iov, 1, -1, on_read);
	// *outfd = out_uv.result;

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
	printf("\t-o, --out=<path>    write logs to file [default: /dev/stderr]\n");
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

	if ((ret = files_init(l->loop, args.input_file, args.output_file)) != 0) {
		perror("files_init");
		goto exit;
	}

	ret = uv_run(l->loop, UV_RUN_DEFAULT);

exit:
	files_close(l->loop, infd, outfd);
	uv_stop(l->loop);
	buf_free(b);
	parser_free(p);
	lua_free(l);
	return ret;
}
