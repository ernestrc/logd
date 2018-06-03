#Logd[![Build Status](https://travis-ci.org/ernestrc/logd.svg)](https://travis-ci.org/ernestrc/logd)
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
| `function logd.on_eof ()` | Called when collector has reached EOF reading the input file. The program will exit after this function returns.  |
| `function logd.on_error (error, logptr, at)` | Called when collector failed to parse a log line. Parsing will resume after this function returns. |

## Parsing
The builtin parser expects logs to be in the following format:
```
YYYY-MM-dd hh:mm:ss	LEVEL	[Thread]	Class	key: value...
```
If you need to parse logs with a different format, you can load a dynamic shared object that implements [src/parser.h](src/parser.h)  via `--parser` flag.

## Luvit
Logd is compatible with [Luvit](https://luvit.io) modules. Luvit modules are not preloaded by default so in order to install the Luvit runtime you can do so with `lit install luvit/luvit` in your script's directory. Then inside your script, you need to call `logd.init_luvit()` to set it up.
