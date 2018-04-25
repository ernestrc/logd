default: deps

override undefine CFLAGS

SLAB_TREE=2c43ec8117b7dc4371ba32e4eb9a7f5b3c9c5ae9
LUAJIT_VERSION=2.0.5

LIB = lib
LIBLUAJIT = lua/src/libluajit.so
LIBSLAB = $(LIB)/libslab.a
INCLUDE_LUAJIT=../src/lua
INCLUDE_SLAB=../src/slab
LUAJIT_TAR=LuaJIT-$(LUAJIT_VERSION).tar.gz

.PHONY: purge

deps: $(LIB) $(LIBLUAJIT) $(LIBSLAB) $(INCLUDE_SLAB) $(INCLUDE_LUAJIT)

$(LIB):
	@ mkdir $(LIB)

$(INCLUDE_LUAJIT): $(LIBLUAJIT)
	@ mkdir $(INCLUDE_LUAJIT) 
	@ cp lua/src/*.h $(INCLUDE_LUAJIT)

$(LIBLUAJIT):
	@ wget https://luajit.org/download/$(LUAJIT_TAR)
	@ tar -xzvf $(LUAJIT_TAR)
	@ mv LuaJIT-$(LUAJIT_VERSION) lua
	@ cd lua && $(MAKE)
	@ cp lua/src/*.o $(LIB)

$(INCLUDE_SLAB): $(LIBSLAB)
	@ mkdir -p $(INCLUDE_SLAB)
	@ cp slab/src/*.h $(INCLUDE_SLAB)

$(LIBSLAB): export CC = clang
$(LIBSLAB):
	@ echo DOWNLOADING slab
	@ git clone --quiet https://github.com/ernestrc/slab.git slab
	@ cd slab && git checkout $(SLAB_TREE)
	@ echo COMPILING slab
	@ cd slab && $(MAKE)
	@ cp slab/src/*.o $(LIB) && cp slab/lib/libslab.a $(LIBSLAB)

purge:
	@- rm -rf slab
	@- rm -rf lua
	@- rm -rf $(INCLUDE_SLAB)
	@- rm -rf $(INCLUDE_LUAJIT)
	@- rm -rf $(LIB)
	@- rm -f *.tar.gz*
