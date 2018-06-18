AC_PREREQ([2.69])
AC_INIT(logd, 1.0, ernest@unstable.build)

root_dir=$(pwd)
build_libs=""
system_libs="-lm -ldl"
libs_ldflags="-L$root_dir/deps/lib"
libs_cflags="-I$root_dir/deps/include"

AC_DEFUN([SET_DEFLAG],[test -z "$$1" && $1="$2";])
AC_DEFUN([CHECK_BUILD],[
	if test "$build_$2" == yes
	then
		build_libs="$build_libs $root_dir/deps/lib/$2$3.a"
	else
		system_libs="$(pkg-config $1 --libs) $system_libs"
	fi
])
AC_DEFUN([CHECK_LIB],[
	if test "$build_$1" == no
	then
		$2
	fi
])

AC_DEFUN([SET_DEFLAG_DEFINE],[test -z "$$1" && $1="$2"; AC_DEFINE($1, $2) AC_SUBST($1, $2)])

AC_ARG_ENABLE(static, [AS_HELP_STRING([--enable-static-musl], [statically link executable with system musl libc])], [enable_static=yes], [enable_static=no])
AC_ARG_ENABLE(develop, [AS_HELP_STRING([--enable-develop], [enable development build. This sets clang as the compiler and sets a bunch of testing and debugging compiler flags.])], [enable_develop=yes], [enable_develop=no])
AC_ARG_ENABLE(build-libuv, [AS_HELP_STRING([--enable-build-libuv], [build local libuv])], [build_libuv=yes], [build_libuv=no])
AC_ARG_ENABLE(build-luajit, [AS_HELP_STRING([--enable-build-luajit], [build local luajit])], [build_libluajit=yes], [build_libluajit=no])
AC_ARG_ENABLE(build-zlib, [AS_HELP_STRING([--enable-build-zlib], [build local zlib])], [build_libz=yes], [build_libz=no])
AC_ARG_ENABLE(build-pcre, [AS_HELP_STRING([--enable-build-pcre], [build local pcre])], [build_libpcre=yes], [build_libpcre=no])
AC_ARG_ENABLE(build-openssl, [AS_HELP_STRING([--enable-build-openssl], [build local openssl])], [build_libopenssl=yes], [build_libopenssl=no])
AC_ARG_ENABLE(build-all, [AS_HELP_STRING([--enable-build-all], [build all local dependencies])], [build_all=yes], [build_all=no])

AC_CHECK_PROG(PKGCFG,pkg-config,yes)
if test x"$PKGCFG" != x"yes" ; then
    AC_MSG_ERROR([Please install pkg-config before building logd.])
fi

if test "$enable_static" == yes
then
	build_all=yes
	AC_CHECK_PROG(MUSL_CHECK,musl-gcc,yes)
	if test x"$MUSL_CHECK" != x"yes" ; then
		AC_MSG_ERROR([Please install musl libc before building with --enable-static.])
	fi
	AC_MSG_WARN([THIS IS EXPERIMENTAL. USE THIS BUILD AT YOUR OWN RISK.])
fi

if test "$build_all" == yes
then
	build_libz=yes
	build_libpcre=yes
	build_libuv=yes
	build_libluajit=yes
	build_libopenssl=yes
fi

CHECK_BUILD(zlib, libz)
CHECK_BUILD(libpcre, libpcre)
CHECK_BUILD(libuv, libuv)
CHECK_BUILD(luajit, libluajit, -5.1)

# openssl builds two separate .a files
if test "$build_libopenssl" == yes
then
	build_libs="$root_dir/deps/lib/libssl.a $root_dir/deps/lib/libcrypto.a $build_libs"
else
	system_libs="$(pkg-config openssl --libs) $system_libs"
fi

# add -I/usr/local/include/luajit
if test "$build_libluajit" == no
then
	libs_cflags="$libs_cflags $(pkg-config luajit --cflags-only-I)"
	luajit=/usr/bin/luajit
else
	# TODO add instrumentation luajit="LUA_PATH=\"$root_dir/deps/luajit/src/?.lua;$root_dir/deps/luajit/src/jit/?.lua\" $root_dir/deps/luajit/src/luajit"
	libs_cflags="$libs_cflags -I$root_dir/deps/include/luajit-2.0"
	luajit=/usr/bin/luajit
fi

if test "$build_libopenssl" == yes
then
	AC_DEFINE(OPENSSL_NO_SM2)
fi

if test "$enable_develop" == yes
then
	# TODO luajit="LUA_PATH=\"$root_dir/deps/luajit/src/?.lua;$root_dir/deps/luajit/src/jit/?.lua\" $root_dir/deps/luajit/src/luajit"
	SET_DEFLAG_DEFINE(LOGD_SLAB_CAP, 30)
	SET_DEFLAG_DEFINE(LOGD_BUF_INIT_CAP, 64000)
	SET_DEFLAG_DEFINE(LOGD_BUF_MAX_CAP, 1000000)

	SET_DEFLAG(CC, [clang])
	SET_DEFLAG(CFLAGS,[$libs_cflags -fsanitize=undefined -Wall -fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping])
	SET_DEFLAG(LDFLAGS,[$libs_ldflags])
	SET_DEFLAG_DEFINE(FUZZFLAGS,[-max_total_time=60]) # fuzzer flags
	AC_DEFINE(LOGD_DEBUG)
else
	SET_DEFLAG_DEFINE(LOGD_SLAB_CAP, 256)
	SET_DEFLAG_DEFINE(LOGD_BUF_INIT_CAP, 1638400)
	SET_DEFLAG_DEFINE(LOGD_BUF_MAX_CAP, 8192000)
	AC_DEFINE(LOGD_INLINE)
	if test "$enable_static" == yes
	then
		SET_DEFLAG(CC, [musl-gcc])
		SET_DEFLAG(CFLAGS,[$libs_cflags -O3 -flto])
		SET_DEFLAG(LDFLAGS,[-static $libs_ldflags])
	else
		SET_DEFLAG(CFLAGS,[$libs_cflags -O3 -flto])
		SET_DEFLAG(LDFLAGS,[$libs_ldflags])
	fi
fi

AC_CHECK_LIB(lua, lua_getglobal, [AC_MSG_ERROR([Please uninstall lua before compiling and make sure luajit is installed.])])

SET_DEFLAG(LIBS,["$system_libs"])
SET_DEFLAG(BUILD_LIBS,["$build_libs"])

AC_LANG(C)
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_USE_SYSTEM_EXTENSIONS

AC_CHECK_PROG(OBJDUMP_CHECK,objdump,yes)
if test x"$OBJDUMP_CHECK" != x"yes" ; then
    AC_MSG_ERROR([Please install binutils before building logd.])
fi

CHECK_LIB(libuv, [AC_CHECK_LIB(uv, uv_loop_close, [], [AC_MSG_ERROR([libuv not installed])])])
CHECK_LIB(libluajit, [AC_CHECK_LIB(luajit-5.1, luaJIT_setmode, [], [AC_MSG_ERROR([luajit not installed])])])
CHECK_LIB(libpcre, [AC_CHECK_LIB(pcre, pcre_compile, [], [AC_MSG_ERROR([pcre not installed])])])
CHECK_LIB(libz, [AC_CHECK_LIB(z, gzread, [], [AC_MSG_ERROR([zlib not installed])])])
CHECK_LIB(libopenssl, [
	AC_CHECK_LIB(crypto, CRYPTO_new_ex_data, [], [AC_MSG_ERROR([library 'crypto' is required for OpenSSL])])
	FOUND_SSL_LIB="no"
	AC_CHECK_LIB(ssl, OPENSSL_init_ssl, [FOUND_SSL_LIB="yes"])
	AC_CHECK_LIB(ssl, SSL_library_init, [FOUND_SSL_LIB="yes"])
	AS_IF([test "x$FOUND_SSL_LIB" = xno], [AC_MSG_ERROR([library 'ssl' is required for OpenSSL])])
])

AC_CHECK_HEADERS([assert.h dlfcn.h errno.h fcntl.h getopt.h libgen.h stdbool.h stdint.h stdlib.h stdio.h string.h sys/stat.h time.h unistd.h utime.h], [], AC_MSG_ERROR([system header not found]))
AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR([dl not available.])])
AC_CHECK_LIB(m, acos, [], [AC_MSG_ERROR([math not available])])
AC_CHECK_LIB(pthread, pthread_create, [], [AC_MSG_ERROR([pthread not available])])

# TODO open up for configuration and add to usage string
AC_DEFINE(WITH_OPENSSL)
AC_DEFINE(WITH_ZLIB)
AC_DEFINE(WITH_PCRE)

if test "$enable_static" == yes
then
	AC_CHECK_HEADERS([linux/mman.h], [], AC_MSG_ERROR([musl linux headers not found. Please install to continue.]))
fi

AC_CONFIG_SRCDIR(src/logd_module.c)

AC_SUBST(LUAJITBIN, ["$luajit"])
AC_SUBST([BUILD_LIBUV], ["$build_libuv"])
AC_SUBST([BUILD_LUAJIT], ["$build_libluajit"])
AC_SUBST([BUILD_ZLIB], ["$build_libz"])
AC_SUBST([BUILD_PCRE], ["$build_libpcre"])
AC_SUBST([BUILD_OPENSSL], ["$build_libopenssl"])
AC_SUBST([DEVELOP_BUILD], ["$enable_develop"])
AC_SUBST([LIBS], ["$LIBS"])
AC_SUBST([BUILD_LIBS], ["$BUILD_LIBS"])
AC_SUBST([EXTRA_CFLAGS], ["$EXTRA_CFLAGS"])
AC_SUBST([EXTRA_LDFLAGS], ["$EXTRA_LDFLAGS"])
AC_SUBST([ROOT_DIR], ["$root_dir"])

AC_MSG_NOTICE([using enable_develop=$enable_develop])
AC_MSG_NOTICE([using enable_static=$enable_static])
AC_MSG_NOTICE([using ROOT_DIR=$root_dir])
AC_MSG_NOTICE([using CC=$CC])
AC_MSG_NOTICE([using CFLAGS=$CFLAGS])
AC_MSG_NOTICE([using LDFLAGS=$LDFLAGS])
AC_MSG_NOTICE([using LIBS=$LIBS])
AC_MSG_NOTICE([using BUILD_LIBS=$BUILD_LIBS])

AC_CONFIG_FILES([src/Makefile
				 deps/Makefile
                 test/Makefile])

AC_CONFIG_HEADERS(src/config.h)

AC_OUTPUT
