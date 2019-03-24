#include <dlfcn.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include "./config.h"
#include "./lua.h"
#include "./scanner.h"
#include "./util.h"

#define DEFAULT_MAX_FILE_MEM ((size_t)9e+8) /* 900 mb */

enum map_state_e { MAP_MAX, MAP_REMAINING };

typedef struct mmap_s {
	size_t map_size;
	size_t real_size;
	enum map_state_e state;
} mmap_t;

int pret;
void* scanner;
void* dlscanner_handle;
lua_t* lstate;
uv_loop_t* loop;
struct uri_s uri;

long PAGE_SIZE_MASK;
long PAGE_SIZE;
size_t MAX_FILE_MEM;

mmap_t mem_map;
int mem_lock_mask;
int mapped_fd;
struct stat* mapped_stat;
char* mapped_addr;
off_t mapped_offset;
off_t log_offset;

scan_res_t (*scan_scanner)(
  void*, char*, size_t) = (scan_res_t(*)(void*, char*, size_t))(&scanner_scan);
void (*free_scanner)(void*) = (void (*)(void*))(&scanner_free);
void* (*create_scanner)() = (void* (*)())(&scanner_create);
void (*reset_scanner)(void*) = (void (*)(void*))(&scanner_reset);

struct args_s {
	const char* script;
	const char* target;
	const char* dlscanner;
	size_t max_file_mem;
} args;

void print_version() { printf("logmr %s\n", LOGD_VERSION); }

// TODO skip when line too long: test_malicious.sh should pass
void print_usage(int argc, char* argv[])
{
	printf("usage: %s <script> <target> [options]\n", argv[0]);
	printf("\narguments:\n");
	printf("  script		Lua script to process log file.\n");
	printf("  target		Target file URI <proto>://[user@]host[:port][/path]\n");
	printf("\n");
	printf("\noptions:\n");
	printf("  -s, --scanner=<file>	Load SO scanner via dlopen [builtin: "
		   "%s]\n",
	  LOGD_BUILTIN_SCANNER);
	//  that if this is to be raised above this process RLIMIT_MEMLOCK's hard
	//  limit, the process needs to be privileged or possess CAP_SYS_RESOURCE.
	printf("  -m, --max-mem=<num>	Max file chunk to be memory mapped"
		   " [default: %ld b]\n",
	  MAX_FILE_MEM);
	printf("  -h, --help		Display this message.\n");
	printf("  -v, --version		Display logmr version information.\n");
	printf("\n");
}

int args_init(int argc, char* argv[])
{
	args.dlscanner = NULL;
	args.max_file_mem = MAX_FILE_MEM;

	ssize_t max_file_mem = 0;

	static struct option long_options[] = {{"help", no_argument, 0, 'h'},
	  {"max-memory", required_argument, 0, 'm'},
	  {"scanner", required_argument, 0, 'p'}, {"version", no_argument, 0, 'v'},
	  {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	while ((c = getopt_long(
			  argc, argv, "m:s:vh", long_options, &option_index)) != -1) {
		switch (c) {
		case 'v':
			print_version();
			exit(0);
		case 'm':
			if ((max_file_mem = parse_non_negative_int(optarg)) == -1) {
				return 1;
			}
			args.max_file_mem = (size_t)max_file_mem & PAGE_SIZE_MASK;
			break;
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

	DEBUG_LOG(
	  "msg: parsed args, script: %s, target: %s, max_mem: %zu, dlscanner: %s",
	  args.script, args.target, args.max_file_mem, args.dlscanner);

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

mmap_t next_mmap_size()
{
	/* pages remaining */
	size_t remaining =
	  (mapped_stat->st_size & PAGE_SIZE_MASK) + PAGE_SIZE - mapped_offset;

	if (remaining > args.max_file_mem) {
		return (mmap_t){
		  args.max_file_mem,
		  args.max_file_mem,
		  MAP_MAX,
		};
	}

	if (remaining > PAGE_SIZE) {
		return (mmap_t){
		  remaining,
		  remaining,
		  MAP_MAX,
		};
	}

	return (mmap_t){
	  PAGE_SIZE,
	  mapped_stat->st_size - mapped_offset,
	  MAP_REMAINING,
	};
}

int mmap_next()
{
	if (mapped_addr != NULL) {
		munmap(mapped_addr, mem_map.map_size);
	}

	mem_map = next_mmap_size();
	DEBUG_LOG("ready for memory mapping,"
			  "fd: %d, file_size: %ld, "
			  "next_map.map_size: %ld, "
			  "next_map.real_size: %ld, offset: %ld, log_offset: %ld, "
			  "state: %d",
	  mapped_fd, mapped_stat->st_size, mem_map.map_size, mem_map.real_size,
	  mapped_offset, log_offset, mem_map.state);

	if ((mapped_addr = (char*)mmap(mapped_addr, mem_map.map_size,
		   PROT_READ | PROT_WRITE, MAP_PRIVATE | mem_lock_mask, mapped_fd,
		   mapped_offset)) == MAP_FAILED) {
		perror("mmap");
		mapped_addr = NULL;
		return 1;
	}

	return 0;
}

// ret -1 err
// ret 0 done
// ret 1 not done
int logmr_file_consume()
{
	scan_res_t res;

	char* next_addr = mapped_addr + log_offset;
	size_t next_addr_len = mem_map.real_size - log_offset;

#define ADD_OFFSET                                                             \
	next_addr += res.consumed;                                                 \
	next_addr_len -= res.consumed;

	reset_scanner(scanner);
	while (1) {
		res = scan_scanner(scanner, next_addr, next_addr_len);
		switch (res.type) {

		case SCAN_COMPLETE:
			res.log->is_safe = true;
			lua_call_on_log(lstate, res.log);
			res.log->is_safe = false;
			ADD_OFFSET;
			reset_scanner(scanner);
			break;

		case SCAN_ERROR:
			DEBUG_LOG("scan error: %s", res.error.msg);
			res.log->is_safe = true;
			if (lua_on_error_defined(lstate)) {
				lua_call_on_error(lstate, res.error.msg, res.log, res.error.at);
			}
			res.log->is_safe = false;
			ADD_OFFSET;
			reset_scanner(scanner);
			break;

		case SCAN_PARTIAL:
			switch (mem_map.state) {
			case MAP_MAX:
				mapped_offset += (mem_map.map_size - PAGE_SIZE);
				log_offset = PAGE_SIZE - next_addr_len;
				if (mmap_next() != 0) {
					return -1;
				}
				return 1;
			case MAP_REMAINING:
				return 0;
			}

			abort();
		}
	}

	abort();
}

int logmr_file_load(const char* pathname)
{

	if (mapped_fd == 0) {
		if ((mapped_fd = open(pathname, 0, O_RDONLY)) < 0) {
			perror("open");
			mapped_fd = 0;
			errno = EINVAL;
			return 1;
		}
		if ((mapped_stat = calloc(1, sizeof(struct stat))) == NULL) {
			return 1;
		}
		if (fstat(mapped_fd, mapped_stat) == -1) {
			perror("fstat");
			errno = EINVAL;
			return 1;
		}
		if (!S_ISREG(mapped_stat->st_mode)) {
			errno = EINVAL;
			return 1;
		}
	}

	if (mmap_next() != 0) {
		errno = EINVAL;
		return 1;
	}

	return 0;
}

void free_all()
{
	if (loop) {
		uv_loop_close(loop);
		free(loop);
	}
	if (mapped_fd) {
		close(mapped_fd);
	}
	if (mapped_stat) {
		free(mapped_stat);
	}
	if (mapped_addr && (munmap(mapped_addr, mem_map.map_size)) == -1) {
		perror("munmap");
	}
	lua_free(lstate);
	free_scanner(scanner);
	if (dlscanner_handle) {
		dlclose(dlscanner_handle);
	}
}

int set_memlock_limit(rlim_t soft, rlim_t hard)
{
	struct rlimit rl;
	int error = 0;

	rl.rlim_cur = soft;
	rl.rlim_max = hard;

	if (setrlimit(RLIMIT_MEMLOCK, &rl) != 0) {
		error = errno;
		DEBUG_LOG("failed to set new RLIMIT_MEMLOCK soft limit of %ld: %s",
		  rl.rlim_max, strerror(errno));
		errno = error;
		return 1;
	}

	DEBUG_LOG("set new RLIMIT_MEMLOCK soft limit of %ld and hard limit of %ld",
	  rl.rlim_cur, rl.rlim_max);

	return 0;
}

size_t get_raise_memlock_lim()
{
	struct rlimit rl;

	mem_lock_mask = MAP_LOCKED;

	if (getrlimit(RLIMIT_MEMLOCK, &rl) != 0) {
		DEBUG_LOG("disabling locking of memory mapped pages: failed to get "
				  "RLIMIT_MEMLOCK: %s",
		  strerror(errno));
		// if we cannot retrieve RLIMIT_MEMLOCK, then do no
		// try to lock memory mapped pages
		mem_lock_mask = 0;
		return DEFAULT_MAX_FILE_MEM & PAGE_SIZE_MASK;
	}

	if (rl.rlim_max > rl.rlim_cur) {
		// try to raise current RLIMIT_MEMLOCK limit but
		// if we fail, ignore error and use current
		if (set_memlock_limit(rl.rlim_max, rl.rlim_max) != 0)
			return rl.rlim_cur;
	}

	return rl.rlim_max;
}

int main(int argc, char* argv[])
{
	int data_left, io_left;
	const char* reason_str = "reached EOF";
	enum exit_reason reason = REASON_EOF;

	PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
	PAGE_SIZE_MASK = ~(PAGE_SIZE - 1);
	MAX_FILE_MEM = get_raise_memlock_lim();

	if (((pret = args_init(argc, argv)) != 0)) {
		print_usage(argc, argv);
		goto exit;
	}

	if (set_memlock_limit(args.max_file_mem, args.max_file_mem) != 0) {
		perror("set_memlock_limit");
		pret = 1;
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

	while ((data_left = logmr_file_consume()) == 1)
		io_left = uv_run(loop, UV_RUN_NOWAIT);

	if (data_left == -1) {
		pret = 1;
		reason_str = strerror(errno);
		reason = REASON_ERROR;
	}

	if (lua_on_exit_defined(lstate)) {
		lua_call_on_exit(lstate, reason, reason_str);
	}

	while (uv_run(loop, UV_RUN_DEFAULT))
		;

exit:
	free_all();
	DEBUG_LOG(
	  "process with pid %d released all resources and exit with status %d",
	  getpid(), pret);
	return pret;
}
