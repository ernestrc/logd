#include <stdio.h>
#include <unistd.h>

#include "parser.h"
#include "slab/buf.h"
#include "util.h"

#define STDIN 0
#define BUF_CAP 10000

int main(int argc, const char* argv[])
{
	parser_t* p = NULL;
	buf_t* b = NULL;
	ssize_t n = 0;
	presult_t res = {0};

	if ((p = parser_create()) == NULL) {
		perror("parser_create()");
		goto error;
	}

	if ((b = buf_create(BUF_CAP)) == NULL) {
		perror("buf_create()");
		goto error;
	}

	for (;;) {
		n = read(STDIN, b->next_write, buf_writable(b));
		switch (n) {
		case 0:
			return 0;
		case -1:
			perror("read()");
			goto error;
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
		}
	}

	return 0;

error:
	parser_free(p);
	buf_free(b);
	return 1;
}
