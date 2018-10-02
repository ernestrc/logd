AC_PREREQ([2.69])
AC_INIT(logd, 1.0, ernest@unstable.build)

root_dir=$(pwd)
build_libs=""
system_libs="-lm -ldl"
libs_ldflags="-L$root_dir/deps/lib"
libs_cflags="-I$root_dir/deps/include"
include_libluajit=yes
include_libuv=yes

AC_DEFUN([SET_DEFLAG],[test -z "$$1" && $1="$2";])
AC_DEFUN([CHECK_BUILD],[
	if test "$build_$2" == yes
	then
		build_libs="$build_libs $root_dir/deps/lib/$2$3.a"
		libs_ldflags="$libs_ldflags $4"
	else 
		if test "$include_$2" == yes
		then
			system_libs="$(pkg-config $1 --libs-only-l) $system_libs"
			libs_ldflags="$(pkg-config $1 --libs-only-L) $(pkg-config $1 --libs-only-other) $libs_ldflags"
		fi
	fi
])
AC_DEFUN([CHECK_LIB],[
	if test "$build_$1" == no
	then
		if test "$include_$1" == yes
		then
			$2
		fi
	else
		$3
	fi
])
AC_DEFUN([SET_DEFLAG_DEFINE],[test -z "$$1" && $1="$2"; AC_DEFINE($1, $2) AC_SUBST($1, $2)])

AC_ARG_ENABLE(lto, [AS_HELP_STRING([--enable-lto], [enable link time optimizations (-flto). If custom CFLAGS are passed, this option has no effect.])], [enable_lto=yes], [enable_lto=no])
AC_ARG_ENABLE(develop, [AS_HELP_STRING([--enable-develop], [enable development build. This sets clang as the compiler and sets a bunch of testing and debugging compiler flags.])], [enable_develop=yes], [enable_develop=no])
AC_ARG_ENABLE(build-all, [AS_HELP_STRING([--enable-build-all], [build all local dependencies])], [build_all=yes], [build_all=no])

AC_ARG_ENABLE(build-libuv, [AS_HELP_STRING([--enable-build-libuv], [build local libuv])], [build_libuv=yes], [build_libuv=no])
AC_ARG_ENABLE(build-luajit, [AS_HELP_STRING([--enable-build-luajit], [build local luajit])], [build_libluajit=yes], [build_libluajit=no])
AC_ARG_ENABLE(build-zlib, [AS_HELP_STRING([--enable-build-zlib], [build local zlib])], [build_libz=yes], [build_libz=no])
AC_ARG_ENABLE(build-openssl, [AS_HELP_STRING([--enable-build-openssl], [build local openssl])], [build_libopenssl=yes], [build_libopenssl=no])

AC_ARG_WITH(zlib, [AS_HELP_STRING([--without-zlib], [do not include zlib as system or local dependency. This overrides --enable-build-zlib])], [include_libz=no], [include_libz=yes])
AC_ARG_WITH(openssl, [AS_HELP_STRING([--without-openssl], [do not include openssl as system or local dependency. This overrides --enable-build-openssl])], [include_libopenssl=no], [include_libopenssl=yes])
AC_ARG_WITH(lpeg, [AS_HELP_STRING([--without-lpeg], [do not include lpeg module.])], [include_lpeg=no], [include_lpeg=yes])

AC_ARG_WITH(builtin-scanner, [AS_HELP_STRING([--with-builtin-scanner], [Build with given source file scanner as builtin scanner.])], [builtin_scanner="$with_builtin_scanner"], [builtin_scanner=default_scanner.c])

AC_CHECK_PROG(PKGCFG,pkg-config,yes)
if test x"$PKGCFG" != x"yes" ; then
    AC_MSG_ERROR([Please install pkg-config before building logd.])
fi

AC_CHECK_PROG(TRUNCPROG,truncate,yes)
if test x"$TRUNCPROG" != x"yes" ; then
    AC_MSG_ERROR([Please install GNU coreutils (or just truncate) truncate before building logd.])
fi

AC_CHECK_PROG(XXDPROG,xxd,yes)
if test x"$XXDPROG" != x"yes" ; then
    AC_MSG_ERROR([Please install xxd before building logd.])
fi

AC_CHECK_PROG(TRPROG,tr,yes)
if test x"$TRPROG" != x"yes" ; then
    AC_MSG_ERROR([Please install GNU coreutils (or just tr) before building logd.])
fi

AC_CHECK_PROG(GITPRG,git,yes)
if test x"$GITPRG" != x"yes" ; then
    AC_MSG_ERROR([Please install git before building logd.])
fi

AC_CHECK_PROG(PATCHPGR,patch,yes)
if test x"$PATCHPGR" != x"yes" ; then
    AC_MSG_ERROR([Please install patch before building logd.])
fi

version=`git describe --tags --long 2>/dev/null || echo "untagged-$(git rev-parse --short HEAD)"`

SET_DEFLAG(TARGET_SYS, [$(uname -s)])


if test "$build_all" == yes
then
	build_libz=yes
	build_libuv=yes
	build_libluajit=yes
	build_libopenssl=yes
fi

# tests require a luajit interpreter with instrumentation
if test "$enable_develop" == yes
then
	build_libluajit=yes
fi

# add zlib lua module
if test "$include_libz" == yes
then
	AC_DEFINE(LOGD_WITH_ZLIB)
	build_libs="$build_libs $root_dir/deps/lib/liblzlib.a"
else
	build_libz=no
fi

# add openssl lua module
if test "$include_libopenssl" == yes
then
	AC_DEFINE(LOGD_WITH_OPENSSL)
	build_libs="$build_libs $root_dir/deps/lib/liblssl.a"
else
	build_libopenssl=no
fi

# add lpeg lua module
if test "$include_lpeg" == yes
then
	AC_DEFINE(LOGD_WITH_LPEG)
	build_libs="$build_libs $root_dir/deps/lib/liblpeg.a"
fi

extra_luajit_ldflags=
if test "$TARGET_SYS" == Darwin
then
	extra_luajit_ldflags="-pagezero_size 10000 -image_base 100000000"
fi

CHECK_BUILD(zlib, libz)
CHECK_BUILD(libuv, libuv)
CHECK_BUILD(luajit, libluajit, -5.1, $extra_luajit_ldflags)

# openssl builds two separate .a files
if test "$build_libopenssl" == yes
then
	build_libs="$root_dir/deps/lib/libssl.a $root_dir/deps/lib/libcrypto.a $build_libs"
else
	if test "$include_libopenssl" == yes
	then
		system_libs="$(pkg-config openssl --libs-only-l) $system_libs"
	fi
fi

# add -I/usr/local/include/luajit
if test "$build_libluajit" == no
then
	libs_cflags="$libs_cflags $(pkg-config luajit --cflags)"
	luajit=/usr/bin/luajit
else
	libs_cflags="$libs_cflags -I$root_dir/deps/include/luajit-2.0"
	luajit="$root_dir/deps/luajit/src/luajit"
fi

lto=
if test "$enable_lto" == yes
then
	lto=-flto
fi

if test "$enable_develop" == yes
then
	SET_DEFLAG_DEFINE(LOGD_SLAB_CAP, 30)
	SET_DEFLAG_DEFINE(LOGD_BUF_INIT_CAP, 64000)
	SET_DEFLAG_DEFINE(LOGD_BUF_MAX_CAP, 1000000)
	SET_DEFLAG_DEFINE(LOGD_PRINT_MAX_KEYS, 50)

	SET_DEFLAG(CC, [clang])
	SET_DEFLAG(EXTRA_CFLAGS,[-fsanitize=undefined -Wall])
	SET_DEFLAG(CFLAGS,[$libs_cflags -fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping])
	SET_DEFLAG_DEFINE(FUZZFLAGS,[-max_total_time=60]) # fuzzer flags
	AC_DEFINE(LOGD_DEBUG)
else
	SET_DEFLAG_DEFINE(LOGD_SLAB_CAP, 256)
	SET_DEFLAG_DEFINE(LOGD_BUF_INIT_CAP, 1638400)
	SET_DEFLAG_DEFINE(LOGD_BUF_MAX_CAP, 8192000)
	SET_DEFLAG_DEFINE(LOGD_PRINT_MAX_KEYS, 100)
	AC_DEFINE(LOGD_INLINE)
	SET_DEFLAG(CFLAGS,[$libs_cflags -O3 $lto])
fi

SET_DEFLAG(LDFLAGS,[$libs_ldflags])

AC_CHECK_LIB(lua, lua_getglobal, [AC_MSG_ERROR([Please uninstall lua before compiling and make sure luajit is installed.])])

SET_DEFLAG(LIBS,["$system_libs"])
SET_DEFLAG(BUILD_LIBS,["$build_libs"])
SET_DEFLAG(LIBTOOLIZE,["libtoolize"])

AC_LANG(C)
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_USE_SYSTEM_EXTENSIONS

AC_CHECK_FILE("src/$builtin_scanner", [AC_MSG_NOTICE([compiling logd with builtin scanner: $builtin_scanner])], [AC_MSG_ERROR([could not find scanner implementation 'src/$builtin_scanner'. Please copy the source file to src folder.])])

AC_CHECK_PROG(OBJDUMP_CHECK,nm,yes)
if test x"$OBJDUMP_CHECK" != x"yes" ; then
    AC_MSG_ERROR([Please make sure that GNU nm is installed before building logd.])
fi

CHECK_LIB(libuv, [
		AC_CHECK_LIB(uv, uv_loop_close, [], [AC_MSG_ERROR([libuv not installed])])
	],[
		AC_CHECK_PROG(AUTOCONFPROG,autoconf,yes)
		if test x"$AUTOCONFPROG" != x"yes" ; then
			AC_MSG_ERROR([Please install autoconf before building libuv.])
		fi

		AC_CHECK_PROG(AUTOMAKEPROG,automake,yes)
		if test x"$AUTOMAKEPROG" != x"yes" ; then
			AC_MSG_ERROR([Please install automake before building libuv.])
		fi

		AC_CHECK_PROG(LIBTOOLIZEPROG,$LIBTOOLIZE,yes)
		if test x"$LIBTOOLIZEPROG" != x"yes" ; then
			AC_MSG_ERROR([Please install $LIBTOOLIZE before building libuv.])
		fi
])
CHECK_LIB(libluajit, [AC_CHECK_LIB(luajit-5.1, luaJIT_setmode, [], [AC_MSG_ERROR([luajit not installed])])], [echo "nothing to check for luajit"])
CHECK_LIB(libz, [AC_CHECK_LIB(z, gzread, [], [AC_MSG_ERROR([zlib not installed])])], [echo "nothing to check for libz"])
CHECK_LIB(libopenssl, [
	AC_CHECK_LIB(crypto, CRYPTO_new_ex_data, [], [AC_MSG_ERROR([library 'crypto' is required for OpenSSL])])
	AC_CHECK_HEADERS([openssl/opensslconf.h], [], AC_MSG_ERROR([openssl headers not found. Please make sure OpenSSL is installed and available in a standard location or pass your own CFLAGS to this script. For example: CFLAGS='$CFLAGS -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib']))
	FOUND_SSL_LIB="no"
	AC_CHECK_LIB(ssl, OPENSSL_init_ssl, [FOUND_SSL_LIB="yes"])
	AC_CHECK_LIB(ssl, SSL_library_init, [FOUND_SSL_LIB="yes"])
	AS_IF([test "x$FOUND_SSL_LIB" = xno], [AC_MSG_ERROR([library 'ssl' is required for OpenSSL])])
], [echo "nothing to check for openssl"])

AC_CHECK_HEADERS([assert.h dlfcn.h errno.h fcntl.h getopt.h libgen.h stdbool.h stdint.h stdlib.h stdio.h string.h sys/stat.h time.h unistd.h utime.h], [], AC_MSG_ERROR([system header not found]))
AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR([dl not available.])])
AC_CHECK_LIB(m, acos, [], [AC_MSG_ERROR([math not available])])
AC_CHECK_LIB(pthread, pthread_create, [], [AC_MSG_ERROR([pthread not available])])

AC_CONFIG_SRCDIR(src/logd_module.c)

AC_SUBST(LUAJITBIN, ["$luajit"])
AC_SUBST([BUILD_LIBUV], ["$build_libuv"])
AC_SUBST([BUILD_LUAJIT], ["$build_libluajit"])
AC_SUBST([BUILTIN_SCANNER], ["$builtin_scanner"])
AC_SUBST([BUILD_ZLIB], ["$build_libz"])
AC_SUBST([BUILD_OPENSSL], ["$build_libopenssl"])
AC_SUBST([DEVELOP_BUILD], ["$enable_develop"])
AC_SUBST([LIBS], ["$LIBS"])
AC_SUBST([BUILD_LIBS], ["$BUILD_LIBS"])
AC_SUBST([EXTRA_CFLAGS], ["$EXTRA_CFLAGS"])
AC_SUBST([CFLAGS], ["$CFLAGS"])
AC_SUBST([EXTRA_LDFLAGS], ["$EXTRA_LDFLAGS"])
AC_SUBST([LDFLAGS], ["$LDFLAGS"])
AC_SUBST([ROOT_DIR], ["$root_dir"])
AC_SUBST(bindir, ["$bindir"])
AC_SUBST(TARGET_SYS, ["$TARGET_SYS"])

AC_MSG_NOTICE([using enable_develop=$enable_develop])
AC_MSG_NOTICE([using enable_lto=$enable_lto])
AC_MSG_NOTICE([using ROOT_DIR=$root_dir])
AC_MSG_NOTICE([using CC=$CC])
AC_MSG_NOTICE([using CFLAGS=$CFLAGS])
AC_MSG_NOTICE([using LDFLAGS=$LDFLAGS])
AC_MSG_NOTICE([using build_libz=$build_libz])
AC_MSG_NOTICE([using build_libuv=$build_libuv])
AC_MSG_NOTICE([using build_libluajit=$build_libluajit])
AC_MSG_NOTICE([using build_libopenssl=$build_libopenssl])
AC_MSG_NOTICE([using include_libz=$include_libz])
AC_MSG_NOTICE([using include_libopenssl=$include_libopenssl])
AC_MSG_NOTICE([using include_lpeg=$include_lpeg])
AC_MSG_NOTICE([using include_libuv=$include_libuv])
AC_MSG_NOTICE([using include_libluajit=$include_libluajit])
AC_MSG_NOTICE([using builtin_scanner=$builtin_scanner])
AC_MSG_NOTICE([using BUILD_LIBS=$BUILD_LIBS])
AC_MSG_NOTICE([using LIBS=$LIBS])

AC_DEFINE_UNQUOTED([LOGD_BUILTIN_SCANNER], ["$builtin_scanner"])
AC_DEFINE_UNQUOTED(LOGD_VERSION, ["$version"])
AC_SUBST(LOGD_VERSION, ["$version"])

AC_CONFIG_FILES([Makefile
				 src/Makefile
				 src/logdconfig.lua
				 test/helper.sh
				 deps/Makefile
                 test/Makefile])

AC_CONFIG_HEADERS(src/config.h)

AC_OUTPUT
