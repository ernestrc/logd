include ../ext/Makefile

override CFLAGS += -Wall -Werror -std=c11 -ggdb $(EXT_FEATURES)

TARGET = ../bin/
SRCS = $(wildcard *.c)
TESTS = $(addprefix $(TARGET),$(patsubst %.c,%,$(SRCS)))
LIB = ../lib/liblogd.a

.PHONY: clean test purge

test: $(TESTS)
	@ for f in $^; do echo RUNNING $$f; ./$$f; done

$(TARGET)%: %.c $(LIB)
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) $< -o $@ $(LIB)

clean:
	@- rm -f $(TESTS)

purge: clean
