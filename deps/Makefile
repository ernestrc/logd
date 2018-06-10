default: deps

PREFIX=$(shell pwd)
LIB = $(PREFIX)/lib
BIN = $(PREFIX)/bin
MAN = $(PREFIX)/man
SHARE = $(PREFIX)/share
INCLUDE = $(PREFIX)/include

LIBC = $(LIB)/libc.a
LIBSLAB = $(LIB)/libslab.a
LIBLUAJIT = $(LIB)/libluajit-5.1.a
LIBUV = $(LIB)/libuv.la
LUA_COMPAT=$(PREFIX)/lua-compat-5.3
LIBLUV = $(LIB)/libluv.a
LPEG = $(LIB)/lpeg.a
LPCRE = $(LIB)/lpcre.a
LIBOPENSSL = $(LIB)/libopenssl.a
LSSL = $(LIB)/lssl.a
LZLIB = $(LIB)/lzlib.a
LIBPCRE = $(LIB)/libpcre.a
ZLIB = $(LIB)/zlib.a

.PHONY: purge

deps: submodules $(LIBC) $(LIBLUAJIT) $(LIBSLAB) $(LIBUV) $(LIBLUV) $(LPEG) $(LPCRE) $(LIBOPENSSL) $(LSSL) $(LZLIB) $(LIBPCRE) $(ZLIB)

submodules:
	@ git submodule update --init --recursive

$(LIBC):
	@ echo COMPILING musl
	@ cd musl && ./configure --prefix=$(PREFIX) && $(MAKE) && $(MAKE) install

$(LIBUV):
	@ echo "  COMPILING	libuv"
	@ cd libuv && ./autogen.sh && ./configure --prefix=$(PREFIX) && $(MAKE) && $(MAKE) install

$(LIBLUAJIT):
	@ echo "  COMPILING	luajit"
	@ sed -i 's/\/usr\/local/$(subst /,\/,$(PREFIX))/g' luajit/Makefile
	@ cd luajit && $(MAKE) && $(MAKE) install

$(LIBOPENSSL):
	@ echo "  COMPILING libopenssl"
	@ cd openssl && $(MAKE)
	@ cp openssl/build-asm/libopenssl.a $(LIBOPENSSL)

$(LIBPCRE):
	@ echo "  COMPILING	libpcre"
	@ cd pcre && ./configure --prefix=$(PREFIX) && $(MAKE) && $(MAKE) install

$(ZLIB):
	@ echo "  COMPILING	zlib"
	@ cd zlib && ./configure --prefix=$(PREFIX) && $(MAKE) && $(MAKE) install


$(LIBSLAB):
	@ echo "  COMPILING	libslab"
	@ cd slab && $(MAKE)
	@ mkdir -p $(LIB) && cp slab/lib/libslab.a $(LIBSLAB)

LUV_WITH_AMALG=OFF
LUV_BUILD_MODULE = OFF
LUV_BUILD_SHARED_LIBS = OFF
LUV_WITH_SHARED_LIBUV = OFF
LUV_WITH_LUA_ENGINE = LuaJIT
LUV_LUA_BUILD_TYPE = Static
CMAKE_OPTIONS = -DWITH_AMALG=$(LUV_WITH_AMALG) \
	-DBUILD_MODULE=$(LUV_BUILD_MODULE) \
	-DBUILD_SHARED_LIBS=$(LUV_BUILD_SHARED_LIBS) \
	-DWITH_SHARED_LIBUV=$(LUV_WITH_SHARED_LIBUV) \
	-DWITH_LUA_ENGINE=$(LUV_WITH_LUA_ENGINE) \
	-DLUA_BUILD_TYPE=$(LUV_LUA_BUILD_TYPE) \
	-DLUA_COMPAT53_DIR=$(LUA_COMPAT) \
	-DCMAKE_INSTALL_PREFIX=$(PREFIX)
$(LIBLUV): $(LIBUV) $(LIBLUAJIT)
	@ echo "  COMPILING	luv"
	@ cp luv.patch luv/luv.patch
	@- cd luv && patch -N -p1 < luv.patch
	@ cd luv && cmake -H. -Bbuild $(CMAKE_OPTIONS) && cmake --build build --target install

$(LPEG):
	@ echo "  COMPILING	lpeg"
	@ cd lpeg && $(MAKE)
	@ cd lpeg && $(AR) rcs $(LPEG) *.o

$(LPCRE):
	@ echo "  COMPILING	lpcre"
	@ cp Makefile.lrexlib lrexlib/Makefile
	@ cd lrexlib && $(MAKE)
	@ cp lrexlib/lpcre.a $(LPCRE)

$(LSSL):
	@ echo "  COMPILING	lua-openssl"
	@ cp Makefile.lssl lua-openssl/Makefile
	@ cd lua-openssl && $(MAKE)
	@ cp lua-openssl/lssl.a $(LSSL)

$(LZLIB):
	@ echo "  COMPILING	lua-zlib"
	@ cd lua-zlib && $(CC) -c $(CFLAGS) -o lua_zlib.o lua_zlib.c
	@ cd lua-zlib/ && $(AR) rcs $(LZLIB) *.o

purge:
	@- rm -rf $(LIB) $(BIN) $(INCLUDE) $(MAN) $(SHARE)
