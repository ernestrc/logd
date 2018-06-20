# Logd [![Build Status](https://travis-ci.org/ernestrc/logd.svg)](https://travis-ci.org/ernestrc/logd)
Logd is a log processor daemon that exposes a lua API to run arbitrary logic on structured logs.

## Logd module API
| Builtin | Description |
| --- | --- |
| `function logd.log_get (logptr, key) value` | Get a property from the log |
| `function logd.log_set (logptr, key, value)` | Set a property to the log |
| `function logd.log_remove (logptr, key)` | Remove a property from a log |
| `function logd.log_reset (logptr)` | Reset all log properties |
| `function logd.to_str (logptr) str` | Serialize a log into a string |
| `function logd.to_logptr (table) logptr` | Instantiate a logptr from a table |
| `function logd.print (string\|table\|logptr)` | Serialize message or table into a log string and print it to stdout |

| Hook | Description |
| --- | --- |
| `function logd.on_log (logptr)` | Logs are parsed and supplied to this handler. Use `logd.log_*` set of functions to manipulate them. |
| `function logd.on_eof ()` | Called when collector has reached EOF reading the input file. The program will exit after this function returns and all libuv handles have been closed.  |
| `function logd.on_error (error, logptr, at)` | Called when collector failed to parse a log line. Parsing will resume after this function returns. |

## Parsing
The builtin parser expects logs to be in the following format:
```
YYYY-MM-dd hh:mm:ss	LEVEL	[Thread]	Class	key: value...
```
If you need to parse logs with a different format, you can load a dynamic shared object that implements [src/parser.h](src/parser.h)  via `--parser` flag.

## Preloaded Lua modules
- [logd](#logd-module-api)
- [uv](https://github.com/luvit/luv)
- [miniz](https://github.com/luvit/luvi/blob/master/src/lminiz.c) 

## Optionally Preloaded Lua modules 
Depending on the build, the following modules are preloaded:
- [lpeg](http://www.inf.puc-rio.br/~roberto/lpeg/)
- [rex](https://github.com/rrthomas/lrexlib)
- [openssl](https://github.com/zhaozg/lua-openssl)
- [zlib](https://github.com/brimworks/lua-zlib)

## Build instructions
Run configure script and then make:
```sh
$ ./configure
$ make
```
If you have problems linking with a system dependency, you can configure the project to build the dependency from source:
```sh
$ ./configure --enable-build-luajit --enable-build-openssl
$ make
```
Alternatively, you can provide your own CFLAGS or LDFLAGS:
```sh
$ ./configure CFLAGS='-DOPENSSL_NO_STDIO -DOPENSSL_NO_SM2'
$ make
```
Finally, you can also disable some of the modules:
```sh
$ ./configure --disable-openssl --disable-lpeg
$ make
```
For a full list of options run `./configure --help`.

## Running tests
Configure and enable the development build:
```sh
$ ./configure --enable-develop
$ make
$ make test
```

## Luvit
Logd uses Libuv under the hood and is compatible with [Luvit](https://luvit.io) modules. The Luvit runtime and standard modules are not preloaded by default but you can do so by running `lit install luvit/luvit` in your script's directory and then supplying your script to the logd executable.
