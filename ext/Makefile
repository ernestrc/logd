default: deps

override undefine CFLAGS

TARGET=../bin

SLAB_TREE=2c43ec8117b7dc4371ba32e4eb9a7f5b3c9c5ae9
LUAJIT_VERSION=2.0.5
LIBUV_TREE=v1.9.1
LUV_TREE=1.9.1-1

LIB = lib
LIBLUAJIT = lua/src/libluajit.so
TESTING_LUAJIT_TARGET = $(shell pwd)/$(TARGET)/luajit # adds gcc instrumentation to use with lua unit tests
TESTING_LUAJIT_BIN=$(shell pwd)/lua_test/src/luajit
LIBUV = libuv/libuv.la
LUVOBJ = lib/luv_luv.o
LIBSLAB = $(LIB)/libslab.a
LUAJIT_TAR=LuaJIT-$(LUAJIT_VERSION).tar.gz
INCLUDE_LUAJIT=../src/lua
INCLUDE_SLAB=../src/slab
INCLUDE_LIBUV=../src/libuv
INCLUDE_LUV=../src/luv

TEST_CFLAGS=-fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping
TEST_CC=clang

.PHONY: purge

deps: $(LIB) $(LIBLUAJIT) $(LIBSLAB) $(INCLUDE_SLAB) $(INCLUDE_LUAJIT) $(INCLUDE_LUV) $(LUVOBJ) $(LIBUV) $(INCLUDE_LIBUV) $(TESTING_LUAJIT_TARGET) 

$(LIB):
	@ mkdir $(LIB)

$(LIBUV):
	@- git clone --quiet https://github.com/libuv/libuv.git libuv
	@ cd libuv && git checkout --quiet $(LIBUV_TREE)
	@ cd libuv && sh autogen.sh && ./configure && $(MAKE)
	@ for file in `ls libuv/src/*.o`; do cp $$file $(LIB)/libuv_`basename $$file`; done
	@ for file in `ls libuv/src/unix/*.o`; do cp $$file $(LIB)/libuv_unix_`basename $$file`; done

$(LUVOBJ): $(INCLUDE_LIBUV) $(INCLUDE_LUAJIT)
	@- git clone --quiet https://github.com/luvit/luv.git luv
	@ cd luv && git checkout --quiet $(LUV_TREE)
	@ cp Makefile.luv luv/src/Makefile
	@ cp luv.patch luv/luv.patch
	@ cd luv && patch -p1 < luv.patch
	@ cd luv/src/ && $(MAKE)
	@ cp luv/src/*.o $(LIB)

$(INCLUDE_LUAJIT): $(LIBLUAJIT)
	@ mkdir -p $(INCLUDE_LUAJIT) 
	@ cp lua/src/*.h $(INCLUDE_LUAJIT)

$(INCLUDE_LUV): $(LUVOBJ)
	@ mkdir -p $(INCLUDE_LUV) 
	@ cp luv/src/*.h $(INCLUDE_LUV)
	@ sed -i 's/<lua.h>/"..\/lua\/lua.h"/g' ../src/luv/luv.h
	@ sed -i 's/<lualib.h>/"..\/lua\/lualib.h"/g' ../src/luv/luv.h
	@ sed -i 's/<lauxlib.h>/"..\/lua\/lauxlib.h"/g' ../src/luv/luv.h
	@ sed -i 's/"uv.h"/"..\/libuv\/uv.h"/g' ../src/luv/luv.h

$(INCLUDE_LIBUV): $(LIBUV)
	@ mkdir -p $(INCLUDE_LIBUV) 
	@ cp libuv/include/*.h $(INCLUDE_LIBUV)

$(LIBLUAJIT):
	@- wget https://luajit.org/download/$(LUAJIT_TAR)
	@ tar -xzvf $(LUAJIT_TAR)
	@ mv LuaJIT-$(LUAJIT_VERSION) lua
	@ cd lua && $(MAKE)
	@ for file in `ls lua/src/*.o`; do cp $$file $(LIB)/luajit_`basename $$file`; done

$(TESTING_LUAJIT_BIN): $(LIBLUAJIT)
	@ tar -xzvf $(LUAJIT_TAR)
	@ mv LuaJIT-$(LUAJIT_VERSION) lua_test
	@ sed -i 's/TARGET_ACFLAGS= $$(CCOPTIONS) $$(TARGET_XCFLAGS) $$(TARGET_FLAGS) $$(TARGET_CFLAGS)/TARGET_ACFLAGS=$(TEST_CFLAGS)/g' lua_test/src/Makefile
	@ sed -i 's/TARGET_ALDFLAGS= $$(LDOPTIONS) $$(TARGET_XLDFLAGS) $$(TARGET_FLAGS) $$(TARGET_LDFLAGS)/TARGET_ALDFLAGS=$(TEST_CFLAGS)/g' lua_test/src/Makefile
	@ sed -i 's/DEFAULT_CC = gcc/DEFAULT_CC = clang/g' lua_test/src/Makefile
	@ cd lua_test && $(MAKE)

$(TESTING_LUAJIT_TARGET): $(TESTING_LUAJIT_BIN)
	@ ln -s $(TESTING_LUAJIT_BIN) $(TESTING_LUAJIT_TARGET)

$(INCLUDE_SLAB): $(LIBSLAB)
	@ mkdir -p $(INCLUDE_SLAB)
	@ cp slab/src/*.h $(INCLUDE_SLAB)

$(LIBSLAB): export CC = clang
$(LIBSLAB):
	@- git clone --quiet https://github.com/ernestrc/slab.git slab
	@ cd slab && git checkout --quiet $(SLAB_TREE)
	@ echo COMPILING slab
	@ cd slab && $(MAKE)
	@ for file in `ls slab/src/*.o`; do cp $$file $(LIB)/libslab_`basename $$file`; done
	@ cp slab/lib/libslab.a $(LIBSLAB)

purge:
	@- rm -rf slab
	@- rm -rf luv
	@- rm -rf libuv
	@- rm -rf lua
	@- rm -rf lua_test
	@- rm -rf $(TESTING_LUAJIT_TARGET)
	@- rm -rf $(INCLUDE_SLAB)
	@- rm -rf $(INCLUDE_LUAJIT)
	@- rm -rf $(INCLUDE_LUV)
	@- rm -rf $(INCLUDE_LIBUV)
	@- rm -rf $(LIB)
	@- rm -f *.tar.gz*
