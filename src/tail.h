#ifndef LOGD_TAIL_H
#define LOGD_TAIL_H

#include <uv.h>

typedef enum tail_state_e {
	IDLE_TSTATE,
	OPEN_TSTATE,
	CLOSING_OPENING_TSTATE,
	CLOSING_FREEING_TSTATE,
	CLOSING_TSTATE,
} tail_state_t;

typedef struct tail_s {
	tail_state_t state;
	uv_loop_t* loop;
	uv_process_t proc;
	uv_process_options_t proc_options;
	uv_stdio_container_t proc_child_stdio[3];
	char* proc_args[5];
	uv_file read_infd;
	uv_file write_infd;
	uv_file read_errfd;
	uv_file write_errfd;
	uv_poll_t uv_poll_err_req;
	void(*exit_cb)(int status);
} tail_t;

tail_t* tail_create(uv_loop_t* loop, char* input_file);
int tail_open(tail_t* tail, void(*exit_cb)(int));
void tail_close(tail_t* tail);
void tail_free(tail_t* tail);

#endif
