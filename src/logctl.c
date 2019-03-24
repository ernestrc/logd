#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "./config.h"
#include "./util.h"

#define AVAIL_CMDS_LEN 2
#define CMD_TAIL "tail"
#define CMD_TAIL_LEN sizeof(CMD_TAIL) - 1
#define CMD_RUN "run"
#define CMD_RUN_LEN sizeof(CMD_RUN) - 1
char* AVAIL_CMDS_DESC[AVAIL_CMDS_LEN] = {
  "Run a collector to process appended data.",
  "Run a collector to process data in batch."};
char* AVAIL_CMDS[AVAIL_CMDS_LEN] = {CMD_TAIL, CMD_RUN};

int pret;

struct args_s {
	const char* command;
} args;

void print_version() { printf("logctl %s\n", LOGD_VERSION); }

// TODO features:
//	1- start/stop/reload new daemonized collector with new script
//	2- codegen from graphql/sql query to lua script
//	3- run graphql/sql queries to logs (by using 2 and starting collector under
//	4- generate random log files of a specified size to run tests/benches
// the hood)
//  5- keep track of log files, pids, etc.
//
// TODO implementation:
// each command is yet another executable in the user PATH in the form of
// logctl-<cmd> except logctl help <cmd> which calls logctl-<cmd> --help and
// logctl start <script> which is an alias for logd <script>

// TODO logq should be the query that uses loggen and this process under the
// hood.
void print_usage(int argc, char* argv[])
{
	printf("usage: %s <cmd> [options]\n", argv[0]);
	printf("\ncommands:\n");
	for (int i = 0; i < AVAIL_CMDS_LEN; i++) {
		printf("  %s\t\t\t%s\n", AVAIL_CMDS[i], AVAIL_CMDS_DESC[i]);
	}
	printf("\noptions:\n");
	printf("  -h, --help		Display this message.\n");
	printf("  -v, --version		Display logctl version information.\n");
	printf("\n");
	printf("Run '%s <command> --help' for more information.\n",
	  argv[0]);
}

/* returns NULL if cmd is not available */
int is_invalid_cmd(const char* cmd)
{
	if (cmd == NULL)
		return 1;

	for (int i = 0; i < AVAIL_CMDS_LEN; i++)
		if (strncmp(AVAIL_CMDS[i], cmd, strlen(AVAIL_CMDS[i])) == 0)
			return 0;

	return 1;
}

int args_init(int argc, char* argv[])
{
	bool version = false;
	bool help = false;

	args.command = argv[1];

	// if valid cmd, then flags should be checked by subprocess
	// otherwise check if argv[1] is one of the flags permitted
	if (is_invalid_cmd(args.command) == 0)
		return 0;

	static struct option long_options[] = {{"help", no_argument, 0, 'h'},
	  {"version", no_argument, 0, 'v'}, {0, 0, 0, 0}};

	int option_index = 0;
	int c = 0;
	while (
	  (c = getopt_long(argc, argv, "vh", long_options, &option_index)) != -1) {
		switch (c) {
		case 'v':
			version = true;
			break;
		case 'h':
			help = true;
			break;
		default:
			abort();
		}
	}

	if (help) {
		print_usage(argc, argv);
		exit(0);
	}

	if (version) {
		print_version();
		exit(0);
	}

	return 1;
}

char** make_argv(int argc, char* argv[])
{
	char** new_argv = NULL;
	char* argv0 = NULL;

	int need = snprintf(NULL, 0, "%s %s", argv[0], args.command);
	if ((argv0 = malloc(need)) == NULL)
		goto error;
	sprintf(argv0, "%s %s", argv[0], args.command);

	// new_argv_len = argc + 1 (add NULL pointer) - 1 (first command is
	// collapsed) = 0
	if ((new_argv = calloc(sizeof(void*), argc)) == NULL)
		goto error;
	for (int i = 2; i < argc; i++) {
		new_argv[i - 1] = argv[i];
	}
	new_argv[0] = argv0;
	new_argv[argc - 1] = NULL;

	return new_argv;

error:
	if (argv0)
		free(argv0);
	if (new_argv)
		free(new_argv);
	return NULL;
}

int exec_sub(const char* path, int argc, char* argv[])
{
	int ret;
	char** new_argv = NULL;

	ret = fork();
	if (ret == -1) {
		perror("fork");
		return 1;
	}

	if (ret != 0) {
		DEBUG_LOG("logctl (pid=%d) waiting for child to finish", getpid());
		if (wait(&ret) == -1) {
			perror("wait");
			return 1;
		}

		if (WIFEXITED(ret)) {
			return WEXITSTATUS(ret);
		}

		if (WIFSIGNALED(ret))
			DEBUG_LOG("child pid exited due to signal %d", WTERMSIG(ret));

		return 1;
	}

	DEBUG_LOG("child pid %d", getpid());
	if ((new_argv = make_argv(argc, argv)) == NULL) {
		ret = 1;
		goto exit;
	}
	execvp(path, new_argv);
	perror("execv");
	printf("there was a problem executing: %s", path);
	ret = 1;

exit:
	if (new_argv) {
		free(new_argv[0]);
		free(new_argv);
	}
	return ret;
}

int main(int argc, char* argv[])
{
	if (((pret = args_init(argc, argv)) != 0)) {
		print_usage(argc, argv);
		goto exit;
	}

	DEBUG_LOG("logctl pid %d", getpid());

	if (strncmp(CMD_TAIL, args.command, CMD_TAIL_LEN) == 0) {
		pret = exec_sub("logd", argc, argv);
	} else if (strncmp(CMD_RUN, args.command, CMD_RUN_LEN) == 0) {
		pret = exec_sub("logb", argc, argv);
	} else {
		// args are checked already by args_init
		abort();
	}

exit:
	DEBUG_LOG(
	  "process with pid %d released all resources and exit with status %d",
	  getpid(), pret);
	return pret;
}
