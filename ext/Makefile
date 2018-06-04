default: deps

override undefine CFLAGS

TARGET=../bin

SLAB_TREE=37f8a5fdf0d7bba2c5f7ec6528caa801e4e00d47
LUAJIT_VERSION=2.0.5
LIBUV_TREE=v1.9.1
LUV_TREE=1.9.1-1
LUVI_TREE=373c4be043a3f036effda9823e37048572521922

LIB = lib
LUVI= $(LIB)/luvi_jitted.a
LIBLUAJIT = lua/src/libluajit.so
LUAJIT_TARGET = $(shell pwd)/$(TARGET)/luajit
LUAJIT_BIN=$(shell pwd)/lua/src/luajit
TESTING_LUAJIT_TARGET = $(shell pwd)/$(TARGET)/luajit-test # adds gcc instrumentation to use with lua unit tests
TESTING_LUAJIT_BIN=$(shell pwd)/lua_test/src/luajit
LIBUV = libuv/libuv.la
LUVOBJ = lib/luv_luv.o
LIBSLAB = $(LIB)/libslab.a
LUAJIT_TAR=LuaJIT-$(LUAJIT_VERSION).tar.gz
INCLUDE_LUAJIT=../src/lua
INCLUDE_SLAB=../src/slab
INCLUDE_LIBUV=../src/libuv
INCLUDE_LUV=../src/luv
INCLUDE_LUVI=../src/luvi

TEST_CFLAGS=-fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping
TEST_CC=clang

.PHONY: purge

deps: $(LIB) $(LUVI) $(LIBLUAJIT) $(LIBSLAB) $(INCLUDE_SLAB) $(INCLUDE_LUAJIT) $(INCLUDE_LUV) $(LUVOBJ) $(LIBUV) $(INCLUDE_LIBUV) $(TESTING_LUAJIT_TARGET) $(INCLUDE_LUVI) $(LUAJIT_TARGET) 

$(LIB):
	@ mkdir $(LIB)

$(LUVI):
	@ echo "  DOWNLOAD	luvi"
	@- git clone --recursive --quiet https://github.com/luvit/luvi.git luvi
	@ cd luvi && git checkout --quiet $(LUVI_TREE)
	@ cp luvi.patch luvi/luvi.patch
	@ cd luvi && patch -p1 < luvi.patch
	@ cd luvi && $(MAKE) regular-asm && $(MAKE)
	@ mkdir -p luvi/build/openssl-objs && cd luvi/build/openssl-objs && ar x ../libopenssl.a
	@ for file in `ls luvi/build/openssl-objs/*.o`; do cp $$file $(LIB)/openssl_`basename $$file`; done
	@ mkdir -p luvi/build/lpeg-objs && cd luvi/build/lpeg-objs && ar x ../libLPEGLIB.a
	@ for file in `ls luvi/build/lpeg-objs/*.o`; do cp $$file $(LIB)/lpeg_`basename $$file`; done
	@ mkdir -p luvi/build/lrex-objs && cd luvi/build/lrex-objs && ar x ../liblrexlib.a
	@ for file in `ls luvi/build/lrex-objs/*.o`; do cp $$file $(LIB)/lrex_`basename $$file`; done
	@ mkdir -p luvi/build/lua-openssl-objs && cd luvi/build/lua-openssl-objs && ar x ../liblua_openssl.a
	@ for file in `ls luvi/build/lua-openssl-objs/*.o`; do cp $$file $(LIB)/lua_openssl_`basename $$file`; done
	@ mkdir -p luvi/build/pcre-objs && cd luvi/build/pcre-objs && ar x ../deps/pcre/libpcre.a
	@ for file in `ls luvi/build/pcre-objs/*.o`; do cp $$file $(LIB)/pcre_`basename $$file`; done
	@ mkdir -p luvi/build/pcre-posix-objs && cd luvi/build/pcre-posix-objs && ar x ../deps/pcre/libpcreposix.a
	@ for file in `ls luvi/build/pcre-posix-objs/*.o`; do cp $$file $(LIB)/pcre_posix_`basename $$file`; done
	@ cd $(LIB) && ar rcus luvi_jitted.a `ls ../luvi/build/jitted_tmp/src/lua/*.o | grep -v init` 2>/dev/null && cp luvi_jitted.a $(LIB)

$(LIBUV):
	@ echo "  DOWNLOAD	libuv"
	@- git clone --quiet https://github.com/libuv/libuv.git libuv
	@ cd libuv && git checkout --quiet $(LIBUV_TREE)
	@ cd libuv && sh autogen.sh && ./configure && $(MAKE)
	@ for file in `ls libuv/src/*.o`; do cp $$file $(LIB)/libuv_`basename $$file`; done
	@ for file in `ls libuv/src/unix/*.o`; do cp $$file $(LIB)/libuv_unix_`basename $$file`; done

$(LUVOBJ): $(INCLUDE_LIBUV) $(INCLUDE_LUAJIT)
	@ echo "  DOWNLOAD	luv"
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

$(INCLUDE_LUVI): $(LUVI)
	@ mkdir -p $(INCLUDE_LUVI) 
	@ cp luvi/src/luvi.h $(INCLUDE_LUVI)
	@ cp luvi/src/lenv.c $(INCLUDE_LUVI)
	@ cp luvi/src/luvi.c $(INCLUDE_LUVI)
	@ cp luvi/src/lminiz.c $(INCLUDE_LUVI)
	@ cp luvi/src/snapshot.c $(INCLUDE_LUVI)
	@ cp luvi/deps/miniz.c $(INCLUDE_LUVI)
	@ sed -i 's/"lua.h"/"..\/lua\/lua.h"/g' ../src/luvi/luvi.h
	@ sed -i 's/"lualib.h"/"..\/lua\/lualib.h"/g' ../src/luvi/luvi.h
	@ sed -i 's/"lauxlib.h"/"..\/lua\/lauxlib.h"/g' ../src/luvi/luvi.h
	@ sed -i 's/"uv.h"/"..\/libuv\/uv.h"/g' ../src/luvi/luvi.h
	@ sed -i 's/"luv.h"/"..\/luv\/luv.h"/g' ../src/luvi/luvi.h
	@ sed -i 's/LUA_VERSION_NUM!=503/false/g' ../src/luvi/luvi.h
	@ sed -i 's/..\/deps\/miniz.c/miniz.c/g' ../src/luvi/lminiz.c
	@ sed -i 's/pFilename; return/return/g' ../src/luvi/miniz.c

$(INCLUDE_LIBUV): $(LIBUV)
	@ mkdir -p $(INCLUDE_LIBUV) 
	@ cp libuv/include/*.h $(INCLUDE_LIBUV)

$(LIBLUAJIT):
	@ echo "  DOWNLOAD	luajit"
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

$(LUAJIT_TARGET): $(LUAJIT_BIN)
	@ ln -s $(LUAJIT_BIN) $(LUAJIT_TARGET)

$(INCLUDE_SLAB): $(LIBSLAB)
	@ mkdir -p $(INCLUDE_SLAB)
	@ cp slab/src/*.h $(INCLUDE_SLAB)

$(LIBSLAB): export CC = clang
$(LIBSLAB):
	@ echo "  DOWNLOAD	slab"
	@- git clone --quiet https://github.com/ernestrc/slab.git slab
	@ cd slab && git checkout --quiet $(SLAB_TREE)
	@ echo COMPILING slab
	@ cd slab && $(MAKE)
	@ for file in `ls slab/src/*.o`; do cp $$file $(LIB)/libslab_`basename $$file`; done
	@ cp slab/lib/libslab.a $(LIBSLAB)

purge:
	@- rm -rf slab
	@- rm -rf luv
	@- rm -rf luvi
	@- rm -rf libuv
	@- rm -rf lua
	@- rm -rf lua_test
	@- rm -rf $(TESTING_LUAJIT_TARGET)
	@- rm -rf $(LUAJIT_TARGET)
	@- rm -rf $(INCLUDE_SLAB)
	@- rm -rf $(INCLUDE_LUAJIT)
	@- rm -rf $(INCLUDE_LUV)
	@- rm -rf $(INCLUDE_LIBUV)
	@- rm -rf $(INCLUDE_LUVI)
	@- rm -rf $(LIB)
	@- rm -f *.tar.gz*
