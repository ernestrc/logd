#Logd[![Build Status](https                                                    \
  : // travis-ci.org/ernestrc/logd.svg)](https://travis-ci.org/ernestrc/logd)
Logd is an open source data collector with an embedded Lua VM. The collector takes care of ingesting and parsing the data and your lua script takes care of transforming it and sending it for storage.

## Lua module API
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
| `function logd.on_error (error, logptr, left)` | Called when collector failed to parse a log line. Parsing will resume after this function returns. |


## Parser
The parser expects logs to be in the following format:
```
YYYY-MM-dd hh:mm:ss	LEVEL	[Thread]	Class	key: value...
```

## Use cases:
- Local aggregation of data
- Dynamic down-sampling
- Event/Error notification
- Log validation and/or sanitization

### TODO
- debug log time should have millis precision
- logd.clone(logptr)
- logd.to\_table(logptr)
- Add query API
- Add directory monitoring flag
