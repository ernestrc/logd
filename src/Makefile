CC ?= clang

LOGD_SLAB_CAP=256
LOGD_BUF_INIT_CAP=1000000
LOGD_BUF_MAX_CAP=10000000

CFLAGS ?= -O3 -std=c11 -DLOGD_INLINE -D_GNU_SOURCE -DLOGD_SLAB_CAP=$(LOGD_SLAB_CAP) -DLOGD_BUF_MAX_CAP=$(LOGD_BUF_MAX_CAP) -DLOGD_INIT_BUF_CAP=$(LOGD_BUF_INIT_CAP) -pthread -flto
LDFLAGS ?= -lm -ldl

INCLUDE_PATH=../include
INCLUDE=$(INCLUDE_PATH)/logd
SYSTEM_LUA_INCLUDE ?= /usr/include/luajit-2.0/ # TODO should use local luajit
SYSTEM_LUA ?= luajit-5.1 # TODO should use local luajit
TARGET =../bin/

EXEC_SRC = logd.c
LUA_MODULE=logd_module.c
LUA_MODULE_DEPS=log.c util.c

EXTOBJ = $(wildcard ../ext/lib/*.o)
SRCS = $(wildcard *.c)
HDRS = $(addprefix ../include/logd/,$(wildcard *.h))
OBJS = $(filter-out $(patsubst %.c,%.o,$(EXEC_SRC)), $(patsubst %.c,%.o,$(SRCS)))
EXEC = $(addprefix $(TARGET),$(patsubst %.c,%,$(EXEC_SRC)))

LIB = ../lib/liblogd.a
SO = ../lib/logd.so

.PHONY: clean purge

src: prepare $(LIB) $(SO) $(EXEC)

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

$(SO): $(OBJS) $(HDRS) $(LUA_MODULE_DEPS)
	@ echo "  SO	$@"
	@ $(CC) $(LDFLAGS) $(CFLAGS) -I$(SYSTEM_LUA_INCLUDE) -I$(INCLUDE_PATH) -shared -fPIC $(LUA_MODULE) -o $(SO) -l$(SYSTEM_LUA) $(LUA_MODULE_DEPS)
	

$(TARGET)%: %.c $(LIB)
	@ echo "  CC	$@"
	@ $(CC) $(LDFLAGS) $(CFLAGS) -I$(INCLUDE_PATH) $< -o $@ $(LIB)

clean:
	@- rm -rf $(INCLUDE_PATH)
	@- rm -f $(EXEC)
	@- rm -f $(HDRS)
	@- rm -f $(LIB)
	@- rm -f $(SO)
	@- rm -f $(OBJS)
	@ rm -f *.profraw
