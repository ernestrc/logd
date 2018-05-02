#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include "parser.h"
#include "slab/buf.h"
#include "util.h"

#define STDIN 0
#define BUF_CAP 100000

// option defaults
#define OPT_DEFAULT_DEBUG false

struct args_s {
	bool debug;
	int infd;
	int outfd;
	int help;
} args;

parser_t* p;
buf_t* b;

int read_all(int fd)
{
	ssize_t n = 0;
	presult_t res = {0};

	for (;;) {
		n = read(fd, b->next_write, buf_writable(b));
		switch (n) {
		case 0:
			return 0;
		case -1:
			perror("read()");
			return 1;
		default:
			buf_extend(b, n);
			res = (presult_t){false, 0};
			for (;;) {
				res = parser_parse(p, b->next_read, buf_readable(b));
				buf_consume(b, res.consumed);
				if (!res.complete) {
					break;
				}
				// TODO hand it off to lua vm
			}
			break;
		}
	}
}

int args_init(int argc, char* argv[])
{
	/* set defaults for arguments */
	args.debug = OPT_DEFAULT_DEBUG;
	args.infd = 0;
	args.outfd = 2;
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
			fd = open(optarg, 0);
			args.infd = fd;
			break;
		case 'o':
			fd = open(optarg, 0);
			args.outfd = fd;
			break;
		case 'h':
			args.help = 1;
			break;
		default:
			abort();
		}
	}

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
	int ret;

	if ((ret = args_init(argc, argv)) == -1 || args.help) {
		print_usage(argv[0]);
		goto exit;
	}

	if ((p = parser_create()) == NULL) {
		perror("parser_create()");
		goto exit;
	}

	if ((b = buf_create(BUF_CAP)) == NULL) {
		perror("buf_create()");
		goto exit;
	}

	ret = read_all(args.infd);

exit:
	parser_free(p);
	buf_free(b);
	return ret;
}
