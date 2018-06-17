AC_PREREQ([2.69])
AC_INIT(logd, 1.0, ernest@unstable.build)

AC_DEFUN([SET_DEFLAG],[test -z "$$1" && $1="$2";])
AC_DEFUN([PKG_LIBS],[`pkg-config $1 --libs`])
AC_DEFUN([PKG_INCLUDE],[`pkg-config $1 --cflags-only-I`])
AC_DEFUN([SET_DEFLAG_DEFINE],[test -z "$$1" && $1="$2"; AC_DEFINE($1, $2) AC_SUBST($1, $2)])

AC_ARG_ENABLE(static, [AS_HELP_STRING([--enable-static], [build local dependencies and statically link final executable with musl libc and all the dependencies])], [enable_static=yes], [enable_static=no])
AC_ARG_ENABLE(develop, [AS_HELP_STRING([--enable-develop], [enable development build. This sets clang as the compiler and sets a bunch of testing and debugging compiler flags.])], [enable_develop=yes], [enable_develop=no])

AC_CHECK_PROG(PKGCFG,pkg-config,yes)
if test x"$PKGCFG" != x"yes" ; then
    AC_MSG_ERROR([Please install pkg-config before building logd.])
fi

if test "$enable_static" == yes
then
	AC_CHECK_PROG(MUSL_CHECK,musl-gcc,yes)
	if test x"$MUSL_CHECK" != x"yes" ; then
		AC_MSG_ERROR([Please install musl libc before building with --enable-static.])
	fi
	AC_MSG_WARN([THIS IS EXPERIMENTAL. USE THIS BUILD AT YOUR OWN RISK.])
fi

if test "$enable_develop" == yes
then
	SET_DEFLAG(CC, [clang])
	SET_DEFLAG(CFLAGS,[-ggdb -I$PWD/deps/include PKG_INCLUDE(luajit) -fsanitize=undefined -Wall -fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping])
	SET_DEFLAG(EXTRA_CFLAGS, [])
	SET_DEFLAG(LDFLAGS, [])
	SET_DEFLAG(LIBS,[PKG_LIBS(libpcre) PKG_LIBS(openssl) PKG_LIBS(libuv) PKG_LIBS(luajit) PKG_LIBS(zlib) -lm -ldl])
	SET_DEFLAG_DEFINE(FUZZFLAGS,[-max_total_time=60]) # fuzzer flags
	# TODO luajit="LUA_PATH=\"$PWD/deps/luajit/src/?.lua;$PWD/deps/luajit/src/jit/?.lua\" $PWD/deps/luajit/src/luajit"
	luajit=/usr/bin/luajit
	SET_DEFLAG_DEFINE(LOGD_SLAB_CAP, 30)
	SET_DEFLAG_DEFINE(LOGD_BUF_INIT_CAP, 64000)
	SET_DEFLAG_DEFINE(LOGD_BUF_MAX_CAP, 1000000)
	AC_DEFINE(LOGD_DEBUG)
else
	SET_DEFLAG_DEFINE(LOGD_SLAB_CAP, 256)
	SET_DEFLAG_DEFINE(LOGD_BUF_INIT_CAP, 1638400)
	SET_DEFLAG_DEFINE(LOGD_BUF_MAX_CAP, 8192000)
	AC_DEFINE(LOGD_INLINE)
	if test "$enable_static" == yes
	then
		SET_DEFLAG(CC, [musl-gcc])
		SET_DEFLAG(CFLAGS,[-fPIC -ggdb -I$PWD/deps/include -I$PWD/deps/include/luajit-2.0])
		SET_DEFLAG(EXTRA_CFLAGS, [-std=c99])
		SET_DEFLAG(LDFLAGS, [-static])
		SET_DEFLAG(EXTRA_LDFLAGS,[-L$PWD/deps/lib])
		SET_DEFLAG(EXTRA_LIBS,[-lpcre -pthread -lssl -lcrypto -ldl -pthread -luv -lrt -lpthread -ldl -lz -lluajit-5.1 -Wl,-E -lm -ldl])
		luajit="LUA_PATH=\"$PWD/deps/luajit/src/?.lua;$PWD/deps/luajit/src/jit/?.lua\" $PWD/deps/luajit/src/luajit"
		AC_DEFINE(OPENSSL_NO_SM2)
	else
		SET_DEFLAG(LIBS,[PKG_LIBS(libpcre) PKG_LIBS(openssl) PKG_LIBS(libuv) PKG_LIBS(luajit) PKG_LIBS(zlib) -lm -ldl])
		SET_DEFLAG(CFLAGS,[-I$PWD/deps/include PKG_INCLUDE(luajit) -O3 -flto])
		luajit=/usr/bin/luajit
	fi
fi

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

# source code system headers
AC_CHECK_HEADERS([assert.h dlfcn.h errno.h fcntl.h getopt.h libgen.h stdbool.h stdint.h stdlib.h stdio.h string.h sys/stat.h time.h unistd.h utime.h], [], AC_MSG_ERROR([system header not found]))

# TODO open up for configuration and add to usage string
AC_DEFINE(WITH_OPENSSL)
AC_DEFINE(WITH_ZLIB)
AC_DEFINE(WITH_PCRE)

if test "$enable_static" == no
then
	AC_CHECK_HEADERS([zlib.h luajit-2.0/lauxlib.h uv.h pcre.h openssl/ssl.h luajit-2.0/lualib.h luajit-2.0/lua.h], [], AC_MSG_ERROR([header not found]))

	AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR([dl not available.])])
	AC_CHECK_LIB(m, acos, [], [AC_MSG_ERROR([math not available])])
	AC_CHECK_LIB(uv, uv_loop_close, [], [AC_MSG_ERROR([libuv not installed])])
	AC_CHECK_LIB(pthread, pthread_create, [], [AC_MSG_ERROR([pthread not available])])
	AC_CHECK_LIB(luajit-5.1, luaJIT_setmode, [], [AC_MSG_ERROR([luajit not installed])])
	AC_CHECK_LIB(lua, lua_getglobal, [AC_MSG_ERROR([Please uninstall lua before compiling and make sure luajit is installed.])])
	AC_CHECK_LIB(z, gzread, [], [AC_MSG_ERROR([zlib not installed])])
	AC_CHECK_LIB(pcre, pcre_compile, [], [AC_MSG_ERROR([pcre not installed])])
	# openssl
	AC_CHECK_LIB(crypto, CRYPTO_new_ex_data, [], [AC_MSG_ERROR([library 'crypto' is required for OpenSSL])])
	FOUND_SSL_LIB="no"
	AC_CHECK_LIB(ssl, OPENSSL_init_ssl, [FOUND_SSL_LIB="yes"])
	AC_CHECK_LIB(ssl, SSL_library_init, [FOUND_SSL_LIB="yes"])
	AS_IF([test "x$FOUND_SSL_LIB" = xno], [AC_MSG_ERROR([library 'ssl' is required for OpenSSL])])
else
	AC_CHECK_HEADERS([linux/mman.h], [], AC_MSG_ERROR([musl linux headers not found. Please install to continue.]))
	AC_TYPE_UID_T
	AC_C_INLINE
	AC_TYPE_INT16_T
	AC_TYPE_INT32_T
	AC_TYPE_INT64_T
	AC_TYPE_INT8_T
	AC_TYPE_MODE_T
	AC_TYPE_OFF_T
	AC_TYPE_PID_T
	AC_TYPE_SIZE_T
	AC_TYPE_SSIZE_T
	AC_CHECK_MEMBERS([struct stat.st_blksize])
	AC_STRUCT_ST_BLOCKS
	AC_CHECK_MEMBERS([struct stat.st_rdev])
	AC_TYPE_UINT16_T
	AC_TYPE_UINT32_T
	AC_TYPE_UINT64_T
	AC_TYPE_UINT8_T
	AC_CHECK_TYPES([ptrdiff_t])
	AC_FUNC_MALLOC
	AC_FUNC_ALLOCA
	AC_FUNC_MMAP
	AC_FUNC_REALLOC
	AC_FUNC_STRERROR_R
	AC_CHECK_FUNCS([__fpending alarm atexit btowc bzero clock_gettime dup2 endgrent endpwent euidaccess fchdir fdatasync fesetround floor ftime ftruncate getcwd getdelim gethostbyaddr gethostbyname gethostname gethrtime getmntent getpagesize getpass getspnam gettimeofday getusershell hasmntopt inet_ntoa isascii iswprint lchown localeconv localtime_r mblen mbrlen memchr memmove mempcpy memset mkdir mkfifo modf munmap nl_langinfo pathconf pow putenv realpath regcomp rint rmdir select setenv sethostname setlocale socket sqrt stime stpcpy strcasecmp strchr strcspn strdup strerror strncasecmp strndup strpbrk strrchr strspn strstr strtol strtoul strtoull strtoumax strverscmp sysinfo tzset uname utime utmpname utmpxname wcwidth])
fi

AC_CONFIG_SRCDIR(src/logd_module.c)

AC_SUBST(LUAJITBIN, ["$luajit"])
AC_SUBST([STATIC_BUILD], ["$enable_static"])
AC_SUBST([DEVELOP_BUILD], ["$enable_develop"])
AC_SUBST([LIBS], ["$LIBS"])
AC_SUBST([EXTRA_LIBS], ["$EXTRA_LIBS"])
AC_SUBST([EXTRA_CFLAGS], ["$EXTRA_CFLAGS"])
AC_SUBST([EXTRA_LDFLAGS], ["$EXTRA_LDFLAGS"])
AC_SUBST([DEV_CFLAGS], ["$DEV_CFLAGS"])

AC_MSG_NOTICE([using enable_develop=$enable_develop])
AC_MSG_NOTICE([using enable_static=$enable_static])
AC_MSG_NOTICE([using CFLAGS=$CFLAGS])
AC_MSG_NOTICE([using LDFLAGS=$LDFLAGS])
AC_MSG_NOTICE([using CC=$CC])
AC_MSG_NOTICE([using LIBS=$LIBS])

AC_CONFIG_FILES([src/Makefile
				 deps/Makefile
                 test/Makefile])

AC_CONFIG_HEADERS(src/config.h)

AC_OUTPUT
