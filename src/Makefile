include ../ext/Makefile

override CFLAGS += -Wall -std=c11 $(EXT_FEATURES)

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

src: $(LIB) $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

$(INCLUDE)/%.h: %.h
	@ cd ../include/logd && \
		ln -s $(subst $(INCLUDE)/,../../src/,$@) $(subst $(INCLUDE)/,,$@)

$(LIB): $(OBJS) $(HDRS)
	@ ar rs $(LIB) $(OBJS) $(EXTOBJ)

$(TARGET)%: %.c $(LIB)
	@ echo COMPILING $@
	@ $(CC) $(CFLAGS) -I$(INCLUDE_PATH) $< -o $@ $(LIB)

clean:
	@- rm -f $(EXEC)
	@- rm -f $(HDRS)
	@- rm -f $(LIB)
	@- rm -f $(OBJS)

purge: clean
