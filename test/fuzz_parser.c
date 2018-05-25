#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/parser.h"

static parser_t* p;
static presult_t res;

int LLVMFuzzerInitialize(int* argc, char*** argv)
{
	p = parser_create();
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	// parser mutates input so we need to make a copy
	char* data_copy;
	data_copy = malloc(size);
	if (data_copy == NULL) {
		perror("malloc");
		return 1;
	}
	memcpy(data_copy, data, size);

	parser_init(p, p->pslab);
	for (;;) {
		res = parser_parse(p, data_copy, size);
		if (!res.complete) {
			break;
		}
	}

	free(data_copy);
	return 0;
}
