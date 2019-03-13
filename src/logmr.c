#include <dlfcn.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "./config.h"
#include "./lua.h"
#include "./scanner.h"
#include "./util.h"

// #define DEFAULT_BUF 300

int pret;
char* log_start;
void* scanner;
void* dlscanner_handle;
lua_t* lstate;
uv_loop_t* loop;
struct uri_s uri;

scan_res_t (*scan_scanner)(
  void*, char*, size_t) = (scan_res_t(*)(void*, char*, size_t))(&scanner_scan);
void (*free_scanner)(void*) = (void (*)(void*))(&scanner_free);
void* (*create_scanner)() = (void* (*)())(&scanner_create);
void (*reset_scanner)(void*) = (void (*)(void*))(&scanner_reset);

struct args_s {
	const char* script;
	const char* target;
	const char* dlscanner;
} args;

void print_version() { printf("logmr %s\n", LOGD_VERSION); }

void print_usage(int argc, char* argv[])
{
	printf("usage: %s <script> <target> [options]\n", argv[0]);
	printf("\narguments:\n");
	printf("\a  script		Lua script to process log file.\n");
	printf("\a  target		Target file URI in the form "
		   "<proto>://[user@]host[:port][/path] where proto can be 'file' or "
		   "'ssh'\n");
	printf("\n");
	printf("\noptions:\n");
	printf("  -s, --scanner=<scanner_so>	Load shared object "
		   "scanner via dlopen [default: "
		   "%s]\n",
	  LOGD_BUILTIN_SCANNER);
	printf("  -h, --help		Display this message.\n");
	printf("  -v, --version		Display logmr version information.\n");
	// TODO maybe its better to memory map option?? printf("  -M, --max-buffer
	// Max target file buffer in MB [default: %d .\n", DEFAULT_BUF);
	printf("\n");
}

int args_init(int argc, char* argv[])
{
	args.dlscanner = NULL;

	static struct option long_options[] = {{"help", no_argument, 0, 'h'},
	  {"scanner", required_argument, 0, 'p'}, {"version", no_argument, 0, 'v'},
	  {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	while ((c = getopt_long(argc, argv, "s:vh", long_options, &option_index)) !=
	  -1) {
		switch (c) {
		case 'v':
			print_version();
			exit(0);
		case 's':
			args.dlscanner = optarg;
			break;
		case 'h':
			print_usage(argc, argv);
			exit(0);
		default:
			abort();
		}
	}

	if (optind + 2 != argc) {
		return 1;
	}
	args.script = argv[optind];
	args.target = argv[optind + 1];

	if (parse_uri(&uri, args.target) != 0) {
		perror("target");
		return 1;
	}

	return 0;
}

int scanner_dlload(const char* dlpath)
{
	const char* error;

	if ((dlscanner_handle = dlopen(dlpath, RTLD_LAZY)) == NULL) {
		fprintf(stderr, "dlopen: %s\n", dlerror());
		goto err;
	}
	dlerror();

	scan_scanner = dlsym(dlscanner_handle, "scanner_scan");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(scanner_scan): %s\n", error);
		goto err;
	}

	free_scanner = dlsym(dlscanner_handle, "scanner_free");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(scanner_free): %s\n", error);
		goto err;
	}

	create_scanner = dlsym(dlscanner_handle, "scanner_create");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(scanner_create): %s\n", error);
		goto err;
	}

	reset_scanner = dlsym(dlscanner_handle, "scanner_reset");
	if ((error = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(scanner_reset): %s\n", error);
		goto err;
	}
	return 0;
err:
	errno = EINVAL;
	return 1;
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

// ret -1 error
// ret 0 done
// ret 1 not done
int logmr_file_consume()
{
	// TODO
	return 0;
}

int logmr_file_load(const char* pathname)
{
	// TODO
	return 0;
}

void free_all()
{
	if (loop) {
		uv_loop_close(loop);
		free(loop);
	}
	lua_free(lstate);
	free_scanner(scanner);
	if (dlscanner_handle) {
		dlclose(dlscanner_handle);
	}
}

int main(int argc, char* argv[])
{
	if (((pret = args_init(argc, argv)) != 0)) {
		print_usage(argc, argv);
		goto exit;
	}

	if (args.dlscanner != NULL &&
	  (pret = scanner_dlload(args.dlscanner)) != 0) {
		perror("scanner_dlload");
		goto exit;
	}

	if ((scanner = create_scanner()) == NULL) {
		perror("scanner_create");
		pret = 1;
		goto exit;
	}

	reset_scanner(scanner);

	if ((pret = loop_create()) != 0) {
		perror("loop_create");
		goto exit;
	}

	if ((pret = uv_loop_init(loop)) < 0) {
		fprintf(stderr, "%s: %s\n", uv_err_name(pret), uv_strerror(pret));
		goto exit;
	}

	if ((lstate = lua_create(loop, args.script)) == NULL) {
		perror("lua_create");
		pret = 1;
		goto exit;
	}

	if (uri.is_remote) {
		// TODO
		abort();
	} else if (logmr_file_load(uri.path) != 0) {
		perror("file_load");
		pret = 1;
		goto exit;
	}

	do {
		DEBUG_LOG("uv_run: %p, logmr pid: %d", lstate, getpid());
	} while ((pret = logmr_file_consume()) == 1 || uv_run(loop, UV_RUN_DEFAULT));

exit:
	free_all();
	DEBUG_LOG(
	  "process with pid %d released all resources and exit with status %d",
	  getpid(), pret);
	return pret;
}
