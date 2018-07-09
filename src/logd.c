#include <dlfcn.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include "./collector.h"
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
	const char* lua_script;
	int help;
	const char* dlparser;
} args;

static int pret;

static collector_t* c;
static uv_signal_t sigusr1, sigint;
static uv_loop_t* loop;

void print_version() { printf("logd %s\n", LOGD_VERSION); }

void free_all()
{
	if (loop) {
		uv_loop_close(loop);
		free(loop);
	}
	collector_free(c);
}

int args_init(int argc, char* argv[])
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
		return 1;
	}

	int mode = uv_stat_in_req.statbuf.st_mode;
	if (S_ISREG(mode) || S_ISDIR(mode)) {
		errno = EINVAL;
		perror("args_init: --file, -f");
		return 1;
	}

	uv_fs_req_cleanup(&uv_stat_in_req);
	if ((args.lua_script = argv[optind]) == NULL) {
		errno = EINVAL;
		perror("args_init");
		return 1;
	}
	return 0;
}

void reload_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Reloading ...", signum);
	// TODO use global function
	collector_close_lua_handles(c);
	uv_stop(loop);
}

void shutdown_sig_h(uv_signal_t* handle, int signum)
{
	DEBUG_LOG("Received signal %d. Shutdown ...", signum);
	// TODO use global function
	collector_close_all_handles(c);
	// TODO uv_signal_stop(&sigusr1);
	// TODO uv_signal_stop(&sigint);
	pret = signum + 128;
}

int signals_init(uv_loop_t* loop)
{
	int ret;

	if ((ret = uv_signal_init(loop, &sigusr1)) < 0)
		goto error;

	if ((ret = uv_signal_init(loop, &sigint)) < 0)
		goto error;

	if ((ret = uv_signal_start(&sigusr1, reload_sig_h, SIGUSR1)) < 0)
		goto error;

	if ((ret = uv_signal_start(&sigint, shutdown_sig_h, SIGINT)) < 0)
		goto error;

	STAMP_HANDLE((uv_handle_t*)&sigusr1, c);
	STAMP_HANDLE((uv_handle_t*)&sigint, c);

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
	if ((args_init(argc, argv)) || args.help) {
		print_usage(argv[0]);
		pret = !args.help;
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

	if ((pret = signals_init(loop)) != 0)
		goto exit;

	if ((c = collector_create(loop, args.dlparser, args.input_file, args.lua_script)) == NULL) {
		perror("collector_create");
		goto exit;
	}

	do {
		collector_load_lua(c);
	} while (uv_run(loop, UV_RUN_DEFAULT));

	DEBUG_ASSERT(uv_loop_alive(loop) == 0);

exit:
	free_all();
	DEBUG_LOG("realeased all resources; exiting with status code: %d", pret);
	return pret;
}
