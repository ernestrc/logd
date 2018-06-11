# TODO move to configure.ac
PWD=$(shell pwd)
DEPS=$(PWD)/../deps
INCDEPS=$(DEPS)/include
BINDEPS=$(DEPS)/bin
LIBDEPS=$(DEPS)/lib

LOGD_SLAB_CAP ?= 256
LOGD_BUF_INIT_CAP ?= 1000000
LOGD_BUF_MAX_CAP ?= 10000000

CFLAGS ?= -I$(INCDEPS)/luajit-2.0 -I$(INCDEPS) -DLOGD_INLINE -D_GNU_SOURCE -DLOGD_SLAB_CAP=$(LOGD_SLAB_CAP) -DLOGD_BUF_MAX_CAP=$(LOGD_BUF_MAX_CAP) -DLOGD_INIT_BUF_CAP=$(LOGD_BUF_INIT_CAP)
LDFLAGS ?= -lm -ldl -lpthread -lluajit-5.1 -lz -lpcre $(shell pkg-config openssl --libs) $(shell pkg-config libuv --libs)
PREFIX ?= $(PWD)/..
LUAJIT ?= $(DEPS)/luajit/src/luajit

INCDIR=$(PREFIX)/include
BINDIR =$(PREFIX)/bin
LIBDIR =$(PREFIX)/lib

EXEC_SRC = logd.c
EXEC_OBJ = logd.o
SRCS = $(wildcard *.c)
LUA_SRCS = $(wildcard *.lua)
OBJS = $(patsubst %.c,%.o,$(SRCS))
JITTED = $(patsubst %.lua,%.jit.o,$(LUA_SRCS))
EXEC = $(addprefix $(BINDIR)/,$(patsubst %.c,%,$(EXEC_SRC)))
STATIC_LIBS = $(LIBDEPS)/libslab.a $(LIBDEPS)/liblssl.a $(LIBDEPS)/libluv.a $(LIBDEPS)/liblpcre.a $(LIBDEPS)/liblpeg.a $(LIBDEPS)/liblzlib.a
CMOD = $(LIBDIR)/logd.so
CMOD_DEPS=log.c util.c
PARSER_SO = $(LIBDIR)/parser.so # TODO rename
PARSER_DEPS=log.c util.c
LIBJITTED = $(LIBDIR)/logdjit.a
LIB = $(LIBDIR)/liblogd.a

listobj = $(shell objdump -f $(1) | grep 'file format' | awk -F':' '{print "$(2)/"$$1}' | xargs echo)
untarlib = $(shell cd $(LIBDIR)/objs/$(1) ; cp $(LIBDEPS)/$(1).a . ; ar x $(1).a)
EXTOBJS = $(addprefix $(LIBDIR)/objs/, $(foreach lib,$(STATIC_LIBS),$(call listobj, $(lib), $(patsubst $(LIBDEPS)/%.a,%,$(lib)))))

.PHONY: clean install

default: src

src: prepare $(LIB) $(LIBJITTED) $(EXEC) $(CMOD) $(PARSER_SO)

prepare:
	@ mkdir -p $(INCDIR)/logd
	@ mkdir -p $(addprefix $(LIBDIR)/objs/,$(patsubst $(LIBDEPS)/%.a,%,$(STATIC_LIBS)))
	@ for lib in $(patsubst $(LIBDEPS)/%.a,%,$(STATIC_LIBS)); do cd $(LIBDIR)/objs/$$lib && cp $(LIBDEPS)/$$lib.a . && ar x $$lib.a; done

%.jit.o: %.lua
	@ echo "  JIT	$@"
	@ cd `dirname $(LUAJIT)` && ./`basename $(LUAJIT)` -b ../../../src/$< ../../../src/`basename $< .lua`.jit.o

%.o: %.c
	@ echo "  CC	$@"
	@ $(CC) $(CFLAGS) -c $<

$(LIBJITTED): $(JITTED)
	@ echo "  AR	$@"
	@ $(AR) rcs $(LIBJITTED) $(JITTED)

$(LIB): $(OBJS) $(EXTOBJS)
	@ echo "  AR	$@"
	@ $(AR) rcs $(LIB) $(OBJS) $(EXTOBJS)

$(BINDIR)/%: %.o $(LIBJITTED) $(OBJS) $(EXTOBJS)
	@ echo "  LD	$@"
	@ $(CC) $(LDFLAGS) -Wl,--whole-archive $(patsubst  -l% ,%.a,$(JITTED)) -Wl,--no-whole-archive -o $@ $(OBJS) $(EXTOBJS) $(LIBJITTED)

$(PARSER_SO): parser.h parser.c $(PARSER_DEPS)
	@ echo "  SO	$@"
	@ $(CC) $(CFLAGS) $(LDFLAGS) -shared -fPIC -o $(PARSER_SO) parser.c $(PARSER_DEPS)

$(CMOD): logd_module.h logd_module.c $(CMOD_DEPS)
	@ echo "  SO	$@"
	@ $(CC) $(CFLAGS) $(LDFLAGS) -shared -fPIC -o $(CMOD) logd_module.c $(CMOD_DEPS)

# TODO
install:
	@ echo TODO

$(INCDIR)/logd/%.h: %.h
	@ cd ../include/logd && \
		ln -s $(subst $(INCDIR)/,../../src/,$@) $(subst $(INCDIR)/,,$@)

clean:
	@- rm -rf $(INCDIR)/logd
	@- rm -rf $(LIBDIR)/objs
	@- rm -f $(EXEC)
	@- rm -f $(LIB)
	@- rm -f $(LIBJITTED)
	@- rm -f $(JITTED)
	@- rm -f $(SO)
	@- rm -f $(OBJS)
	@ rm -f *.profraw
