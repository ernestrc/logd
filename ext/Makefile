default: deps

override undefine CFLAGS

SLAB_TREE=2c43ec8117b7dc4371ba32e4eb9a7f5b3c9c5ae9
LUAJIT_VERSION=2.0.5
LIBUV_TREE=v1.9.1
LUV_TREE=1.9.1-1

LIB = lib
LIBLUAJIT = lua/src/libluajit.so
LIBUV = libuv/libuv.la
LUVOBJ = lib/luv_luv.o
LIBSLAB = $(LIB)/libslab.a
LUAJIT_TAR=LuaJIT-$(LUAJIT_VERSION).tar.gz
INCLUDE_LUAJIT=../src/lua
INCLUDE_SLAB=../src/slab
INCLUDE_LIBUV=../src/libuv
INCLUDE_LUV=../src/luv

.PHONY: purge

deps: $(LIB) $(LIBLUAJIT) $(LIBSLAB) $(INCLUDE_SLAB) $(INCLUDE_LUAJIT) $(INCLUDE_LUV) $(LUVOBJ) $(LIBUV) $(INCLUDE_LIBUV)

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
	@- rm -rf $(INCLUDE_SLAB)
	@- rm -rf $(INCLUDE_LUAJIT)
	@- rm -rf $(INCLUDE_LUV)
	@- rm -rf $(INCLUDE_LIBUV)
	@- rm -rf $(LIB)
	@- rm -f *.tar.gz*
