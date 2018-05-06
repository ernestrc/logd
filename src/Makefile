CC ?= clang
CFLAGS ?= -O2 -std=c11 -DLOGD_INLINE -D_GNU_SOURCE -pthread
LDFLAGS ?= -lm -ldl

INCLUDE_PATH=../include
INCLUDE=$(INCLUDE_PATH)/logd
TARGET =../bin/

EXEC_SRC = logd.c

EXTOBJ = $(wildcard ../ext/lib/*.o)
SRCS = $(wildcard *.c)
HDRS = $(addprefix ../include/logd/,$(wildcard *.h))
OBJS = $(filter-out $(patsubst %.c,%.o,$(EXEC_SRC)), $(patsubst %.c,%.o,$(SRCS)))
EXEC = $(addprefix $(TARGET),$(patsubst %.c,%,$(EXEC_SRC)))

LIB = ../lib/liblogd.a

.PHONY: clean purge

src: prepare $(LIB) $(EXEC)

prepare:
	@ mkdir -p $(INCLUDE)

%.o: %.c
	@ echo "  CC	$@"
	@ $(CC) $(CFLAGS) -c $<

$(INCLUDE)/%.h: %.h
	@ cd ../include/logd && \
		ln -s $(subst $(INCLUDE)/,../../src/,$@) $(subst $(INCLUDE)/,,$@)

$(LIB): $(OBJS) $(HDRS)
	@ echo "  AR	$@"
	@ ar rs $(LIB) $(OBJS) $(EXTOBJ) 2> /dev/null

$(TARGET)%: %.c $(LIB)
	@ echo "  CC	$@"
	@ $(CC) $(LDFLAGS) $(CFLAGS) -I$(INCLUDE_PATH) $< -o $@ $(LIB)

clean:
	@- rm -f $(EXEC)
	@- rm -f $(HDRS)
	@- rm -f $(LIB)
	@- rm -f $(OBJS)

purge: clean
	@- rm -rf $(INCLUDE_PATH)
