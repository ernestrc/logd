#include <stdio.h>
#include "parser.h"

int main(int argc, const char* argv[])
{
	parser_t* p;

	if ((p = parser_create()) == NULL) {
		perror("parser_create()");
		goto error;
	}

	printf("hello world!\n");
	return 0;

error:
	if (p) parser_free(p);
	return 1;
}
