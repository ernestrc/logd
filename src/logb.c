#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "./config.h"
#include "./lua.h"
#include "./scanner.h"
#include "./util.h"

#define DEFAULT_MAX_FILE_MEM ((size_t)9e+8) /* 900 mb */

enum map_state_e { LOGB_MAP_MAX, LOGB_MAP_REM };

struct mmap_s {
	size_t map_size;
	size_t real_size;
	enum map_state_e state;
};

struct mmap_file_s {

	const char* pathname;
	int fd;
	struct stat fstat;

	struct mmap_s mem_map;
	char* addr;
	off_t offset;
	off_t log_offset;
};

struct args_s {
	const char* script;
	struct mmap_file_s* files;
	int files_len;
	const char* dlscanner;
	size_t max_file_mem;
} args;

int pret;
void* scanner;
void* dlscanner_handle;
lua_t* lstate;
uv_loop_t* loop;

int mem_lock_mask;
long page_size_mask;
long page_size;
size_t max_file_mem;

scan_res_t (*scan_scanner)(
  void*, char*, size_t) = (scan_res_t(*)(void*, char*, size_t))(&scanner_scan);
void (*free_scanner)(void*) = (void (*)(void*))(&scanner_free);
void* (*create_scanner)() = (void* (*)())(&scanner_create);
void (*reset_scanner)(void*) = (void (*)(void*))(&scanner_reset);

void print_version() { printf("logb %s\n", LOGD_VERSION); }

// TODO allow take a list of files
// TODO user circular work-stealing dequeue
// https://www.dre.vanderbilt.edu/~schmidt/PDF/work-stealing-dequeue.pdf.
// Implement https://github.com/kinghajj/deque
// TODO use teddy:
// https://github.com/rust-lang/regex/blob/3de8c44f5357d5b582a80b7282480e38e8b7d50d/src/simd_accel/teddy128.rs.
//	Substring search with a Boyer-Moore variant and a well placed memchr to
// quickly skip through the haystack.
// TODO record Lua script filters and skip the rest of lines to speed up scan
void print_usage(int argc, char* argv[])
{
	printf("usage: %s <script> <target> ... [options]\n", argv[0]);
	printf("\narguments:\n");
	printf("  script		Lua script to process log file.\n");
	printf(
	  "  target		Target file(s) path\n");
	printf("\n");
	printf("\noptions:\n");
	printf("  -s, --scanner=<file>	Load SO scanner via dlopen [builtin: "
		   "%s]\n",
	  LOGD_BUILTIN_SCANNER);
	//  that if this is to be raised above this process RLIMIT_MEMLOCK's hard
	//  limit, the process needs to be privileged or possess CAP_SYS_RESOURCE.
	printf("  -m, --max-mem=<num>	Max file chunk to be memory mapped"
		   " [default: %ld b]\n",
	  max_file_mem);
	printf("  -h, --help		Display this message.\n");
	printf("  -v, --version		Display logb version information.\n");
	printf("\n");
}

int args_init(int argc, char* argv[])
{
	args.dlscanner = NULL;
	args.max_file_mem = max_file_mem;
	args.files_len = 0;
	args.files = NULL;

	ssize_t max_file_mem = 0;

	static struct option long_options[] = {{"help", no_argument, 0, 'h'},
	  {"max-memory", required_argument, 0, 'm'},
	  {"scanner", required_argument, 0, 's'}, {"version", no_argument, 0, 'v'},
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
			args.max_file_mem = (size_t)max_file_mem & page_size_mask;
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

	if (argc - 2 < optind) {
		return 1;
	}
	args.script = argv[optind++];
	args.files_len = argc - optind;
	args.files = calloc(args.files_len, sizeof(struct mmap_file_s));

	for (int i = 0; i < args.files_len; i++)
		args.files[i].pathname = argv[optind++];

	DEBUG_LOG(
	  "msg: parsed args, script: %s, targets: %d, max_mem: %zu, dlscanner: %s",
	  args.script, args.files_len, args.max_file_mem, args.dlscanner);

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

INLINE void next_mmap_size(struct mmap_file_s* file)
{
	size_t remaining =
	  (file->fstat.st_size & page_size_mask) + page_size - file->offset;

	if (remaining > max_file_mem) {
		file->mem_map = (struct mmap_s){
		  max_file_mem,
		  max_file_mem,
		  LOGB_MAP_MAX,
		};
		return;
	}

	if (remaining > page_size) {
		file->mem_map = (struct mmap_s){
		  remaining,
		  remaining,
		  LOGB_MAP_MAX,
		};
		return;
	}

	file->mem_map = (struct mmap_s){
	  page_size,
	  file->fstat.st_size - file->offset,
	  LOGB_MAP_REM,
	};
}

int mmap_file_next(struct mmap_file_s* file)
{
	if (file->fd == 0) {
		if ((file->fd = open(file->pathname, 0, O_RDONLY)) < 0) {
			perror("open");
			file->fd = 0;
			errno = EINVAL;
			return 1;
		}
		if (fstat(file->fd, &file->fstat) == -1) {
			perror("fstat");
			errno = EINVAL;
			return 1;
		}
		if (!S_ISREG(file->fstat.st_mode)) {
			errno = EINVAL;
			return 1;
		}
	}

	if (file->addr != NULL)
		munmap(file->addr, file->mem_map.map_size);

	next_mmap_size(file);

	DEBUG_LOG("ready for memory mapping,"
			  "fd: %d, file_size: %ld, "
			  "next_map.map_size: %ld, "
			  "next_map.real_size: %ld, offset: %ld, log_offset: %ld, "
			  "state: %d",
	  file->fd, file->fstat.st_size, file->mem_map.map_size,
	  file->mem_map.real_size, file->offset, file->log_offset,
	  file->mem_map.state);

	if ((file->addr = (char*)mmap(file->addr, file->mem_map.map_size,
		   PROT_READ | PROT_WRITE, MAP_PRIVATE | mem_lock_mask, file->fd,
		   file->offset)) == MAP_FAILED) {
		perror("mmap");
		file->addr = NULL;
		return 1;
	}

	return 0;
}

void mmap_file_close(struct mmap_file_s* file)
{
	if (file->addr != NULL)
		munmap(file->addr, file->mem_map.map_size);

	if (file->fd != 0)
		close(file->fd);
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

// ret -1 err
// ret 0 done
// ret 1 not done
int logb_file_consume(struct mmap_file_s* file)
{
	scan_res_t res;

	char* next_addr = file->addr + file->log_offset;
	size_t next_addr_len = file->mem_map.real_size - file->log_offset;

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
			switch (file->mem_map.state) {
			case LOGB_MAP_MAX:
				file->offset += (file->mem_map.map_size - page_size);
				file->log_offset = page_size - next_addr_len;
				if (mmap_file_next(file) != 0) {
					return -1;
				}
				return 1;
			case LOGB_MAP_REM:
				return 0;
			}

			abort();
		}
	}

	abort();
}

void free_all()
{
	if (loop) {
		uv_loop_close(loop);
		free(loop);
	}
	free(args.files);
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
		return DEFAULT_MAX_FILE_MEM & page_size_mask;
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

	page_size = sysconf(_SC_PAGE_SIZE);
	page_size_mask = ~(page_size - 1);
	max_file_mem = get_raise_memlock_lim();

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

	// TODO implement worker/stealer for file reading, chunking and scanning
	for (int i = 0; i < args.files_len; i++) {
		if (mmap_file_next(&args.files[i]) != 0) {
			pret = 1;
			reason_str = strerror(errno);
			reason = REASON_ERROR;
			break;
		}
		while ((data_left = logb_file_consume(&args.files[i])) == 1) {
			io_left = uv_run(loop, UV_RUN_NOWAIT);
		}
		if (data_left == -1) {
			pret = 1;
			reason_str = strerror(errno);
			reason = REASON_ERROR;
			break;
		}
		mmap_file_close(&args.files[i]);
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
