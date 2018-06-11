#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(logd, 1.0, ernest@unstable.build)
AC_CONFIG_SRCDIR(src/logd_module.c)
AC_USE_SYSTEM_EXTENSIONS

: ${CFLAGS="-O3"}
: ${LDFLAGS=""}

# Checks for programs.
AC_LANG(C)
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_CHECK_PROG(OBJDUMP_CHECK,objdump,yes)
if test x"$OBJDUMP_CHECK" != x"yes" ; then
    AC_MSG_ERROR([Please install binutils before installing.])
fi

# Checks for libraries.
AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR([dl not available.])])
AC_CHECK_LIB(m, acos, [], [AC_MSG_ERROR([math not available])])
AC_CHECK_LIB(z, gzread, [], [AC_MSG_ERROR([zlib not installed])])
AC_CHECK_LIB(pcre, pcre_compile, [], [AC_MSG_ERROR([pcre not installed])])
AC_CHECK_LIB(uv, uv_loop_close, [], [AC_MSG_ERROR([libuv not installed])])
AC_CHECK_LIB(pthread, pthread_create, [], [AC_MSG_ERROR([pthread not available])])
AC_CHECK_LIB(luajit-5.1, luaJIT_setmode, [], [AC_MSG_ERROR([luajit not installed])])
AC_CHECK_LIB(lua, lua_getglobal, [AC_MSG_ERROR([Please uninstall lua before compiling and make sure luajit is installed.])])

with_openssl=yes
if test "$with_openssl" = yes ; then
  dnl Order matters!
  if test "$PORTNAME" != "win32"; then
     AC_CHECK_LIB(crypto, CRYPTO_new_ex_data, [], [AC_MSG_ERROR([library 'crypto' is required for OpenSSL])])
     FOUND_SSL_LIB="no"
     AC_CHECK_LIB(ssl, OPENSSL_init_ssl, [FOUND_SSL_LIB="yes"])
     AC_CHECK_LIB(ssl, SSL_library_init, [FOUND_SSL_LIB="yes"])
     AS_IF([test "x$FOUND_SSL_LIB" = xno], [AC_MSG_ERROR([library 'ssl' is required for OpenSSL])])
  else
     AC_SEARCH_LIBS(CRYPTO_new_ex_data, eay32 crypto, [], [AC_MSG_ERROR([library 'eay32' or 'crypto' is required for OpenSSL])])
     FOUND_SSL_LIB="no"
     AC_SEARCH_LIBS(OPENSSL_init_ssl, ssleay32 ssl, [FOUND_SSL_LIB="yes"])
     AC_SEARCH_LIBS(SSL_library_init, ssleay32 ssl, [FOUND_SSL_LIB="yes"])
     AS_IF([test "x$FOUND_SSL_LIB" = xno], [AC_MSG_ERROR([library 'ssleay32' or 'ssl' is required for OpenSSL])])
  fi
fi

# Checks for header files.
AC_CHECK_HEADERS([OS.h arpa/inet.h fcntl.h float.h inttypes.h limits.h locale.h malloc.h memory.h netdb.h netinet/in.h nl_types.h sgtty.h stddef.h stdint.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/statfs.h sys/statvfs.h sys/systeminfo.h sys/time.h sys/timeb.h syslog.h termio.h termios.h unistd.h utime.h utmp.h utmpx.h])

# Checks for typedefs, structures, and compiler characteristics.
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

# Checks for library functions.
# TODO on static link but dynamic no need to check all of this
AC_FUNC_MALLOC
AC_FUNC_ALLOCA
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([__fpending alarm atexit btowc bzero clock_gettime dup2 endgrent endpwent euidaccess fchdir fdatasync fesetround floor ftime ftruncate getcwd getdelim gethostbyaddr gethostbyname gethostname gethrtime getmntent getpagesize getpass getspnam gettimeofday getusershell hasmntopt inet_ntoa isascii iswprint lchown localeconv localtime_r mblen mbrlen memchr memmove mempcpy memset mkdir mkfifo modf munmap nl_langinfo pathconf pow putenv realpath regcomp rint rmdir select setenv sethostname setlocale socket sqrt stime stpcpy strcasecmp strchr strcspn strdup strerror strncasecmp strndup strpbrk strrchr strspn strstr strtol strtoul strtoull strtoumax strverscmp sysinfo tzset uname utime utmpname utmpxname wcwidth])

luajit=/usr/bin/luajit
incdeps=$PWD/deps/include
AC_SUBST(LUAJITBIN, ["$luajit"])
AC_SUBST([AM_CFLAGS], ["-I$incdeps -I$incdeps/luajit-2.0"])
AC_SUBST([AM_LDFLAGS], ["-lm -ldl -lpthread -lluajit-5.1 -lz -lpcre -lssl -lcrypto -luv -lrt -lnsl"])

# AC_CONFIG_FILES([deps/Makefile
#                  src/Makefile
#                  test/Makefile])
# AC_CONFIG_SUBDIRS([deps/libuv
#                    deps/luv/deps/libuv
#                    deps/musl
#                    deps/pcre
#                    deps/zlib])
# 
AC_CONFIG_HEADERS(src/config.h)

AC_OUTPUT(src/Makefile)
