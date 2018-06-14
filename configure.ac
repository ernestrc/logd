AC_PREREQ([2.69])
AC_INIT(logd, 1.0, ernest@unstable.build)

AC_ARG_ENABLE(static, [AS_HELP_STRING([--disable-static], [use system dependencies and build a dynamically linked binary])], [enable_static=no], [enable_static=yes])
AC_ARG_ENABLE(develop, [AS_HELP_STRING([--enable-develop], [set build type to development. This overrides --disable-static so build always uses local dependencies and statically link binary.])], [enable_develop=yes], [enable_develop=no])
AC_ARG_ENABLE(musl, [AS_HELP_STRING([--enable-musl-libc], [Use musl libc and statically link to it with the final application binary])], [enable_musl_libc=yes], [enable_musl_libc=no])

AC_DEFUN([SET_DEFLAG],[test -z "$$1" && $1="$2";])

SET_DEFLAG(FUZZFLAGS, -max_total_time=60) # fuzzer flags

if test "$enable_musl_libc" == yes
then
	enable_static=yes
    AC_MSG_ERROR([--enable-musl-libc not implemented yet.])
fi

SET_DEFLAG(LIBS, -lm -ldl -lpthread -lluajit-5.1 -lz -lpcre -lssl -lcrypto -luv -lrt -lnsl)
if test "$enable_develop" == yes
then
	enable_static=yes
	SET_DEFLAG(CC, clang)
	SET_DEFLAG(CFLAGS, -I$PWD/deps/include -I$PWD/deps/include/luajit-2.0 -Wall -Wno-unused-function -Werror -fsanitize=undefined -fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping -std=c11 -ggdb)
	SET_DEFLAG(LDFLAGS, -L$PWD/deps/lib)
	luajit="$PWD/deps/bin/luajit"

	AC_DEFINE(LOGD_SLAB_CAP, 32)
	AC_DEFINE(LOGD_BUF_INIT_CAP, 4096)
	AC_DEFINE(LOGD_BUF_MAX_CAP, 409600)
	AC_DEFINE(LOGD_DEBUG)
else
	AC_DEFINE(LOGD_SLAB_CAP, 256)
	AC_DEFINE(LOGD_BUF_INIT_CAP, 1638400)
	AC_DEFINE(LOGD_BUF_MAX_CAP, 8192000)
	AC_DEFINE(LOGD_INLINE)
	if test "$enable_static" == yes
	then
		SET_DEFLAG(CFLAGS, -I$PWD/deps/include -I$PWD/deps/include/luajit-2.0 -O3 -flto)
		SET_DEFLAG(LDFLAGS, -L$PWD/deps/lib)
		luajit="$PWD/deps/bin/luajit"
	else
		SET_DEFLAG(CFLAGS, -I$PWD/deps/include -I/usr/include/luajit-2.0 -O3 -flto)
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
AC_DEFINE(GNU_SOURCE)
AC_USE_SYSTEM_EXTENSIONS

AC_CHECK_PROG(OBJDUMP_CHECK,objdump,yes)
if test x"$OBJDUMP_CHECK" != x"yes" ; then
    AC_MSG_ERROR([Please install binutils before installing.])
fi

# source code system headers
AC_CHECK_HEADERS([assert.h dlfcn.h errno.h fcntl.h getopt.h libgen.h stdbool.h stdint.h stdlib.h stdio.h string.h sys/stat.h time.h unistd.h utime.h], [], AC_MSG_ERROR([system header not found]))

# TODO open up for configuration
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
	AC_CHECK_HEADER_STDBOOL
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
AC_SUBST([USE_MUSL_LIBC], ["$enable_musl_libc"])
AC_SUBST([STATIC_BUILD], ["$enable_static"])
AC_SUBST([DEVELOP_BUILD], ["$enable_develop"])
AC_SUBST([LIBS], ["$LIBS"])

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
