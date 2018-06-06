#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/parser.h"

static parser_t* parser;

int LLVMFuzzerInitialize(int* argc, char*** argv)
{
	parser = parser_create();
	if (parser == NULL) {
		perror("parser_create");
		return 1;
	}

	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	int ret = 0;
	char* data_ptr = NULL;
	parse_res_t res;

	if (size < 0)
		return 0;


	// parser mutates input so we need to make a copy
	data_ptr = malloc(size);
	if (data_ptr == NULL) {
		perror("malloc");
		ret = 1;
		goto end;
	}
	memcpy(data_ptr, data, size);

	char* data_copy = data_ptr;
	parser_reset(parser);
	for (;;) {
		res = parser_parse(parser, data_copy, size);
		switch (res.type) {
		case PARSE_COMPLETE:
			data_copy += res.consumed;
			size -= res.consumed;
			parser_reset(parser);
			break;

		case PARSE_ERROR:
			data_copy += res.consumed;
			size -= res.consumed;
			parser_reset(parser);
			break;

		case PARSE_PARTIAL:
			goto end;
		}
	}

end:
	if (data_ptr)
		free(data_ptr);
	return ret;
}
