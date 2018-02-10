# Logd [![Build Status](https://travis-ci.org/ernestrc/logd.svg)](https://travis-ci.org/ernestrc/logd)
Logd is an open source data collector with an embedded Lua VM. The collector takes care of ingesting and parsing the data and your lua script takes care of transforming it and sending it for storage.

## Lua API
| Builtin | Description |
| --- | --- |
| `function config_set (key, value)` | Set a global configuration property. Check the examples and the Go documentation for more information. Keys that start with `kafka.` will be passed to librdkafka to configure the Kafka producer. Check https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md for more info |
| `function http_get (url, [headers]) body, err` | Perform a blocking HTTP Get request to `url` and return the `body` of the response and/or non-nil `err` if there was an error |
| `function http_post  (url, payload, contentType)` | Perform an HTTP Post request to the given URL with the given `payload` and `Content-Type` header set to `contentType`. Method call is non-blocking unless client is performing back-pressure. Back-pressure can be tuned by changing the http client configuration. Errors will be delivered on the provided `on_http_error` callback if specified |
| `function kafka_offset (key, value)` | Create a new named kafka offset |
| `function kafka_message (key, value, topic, partition, offsetptr) msgptr` | Create a new kafka message and return a pointer to it. `partition` can be set to -1 to use any partition. `offsetptr` can be null |
| `function kafka_produce  (msgptr)` |  Produce a single message. This is an asynchronous call that enqueues the message on the internal transmit queue, thus returning immediately unless Producer is performing back-pressure. The delivery report will be sent on the provided `on_kafka_report` callback if specified |
| `function log_get (logptr, key) value` | Get a property from the log |
| `function log_set (logptr, key, value)` | Set a property to the log |
| `function log_remove (logptr, key)` | Remove a property from a log |
| `function log_reset (logptr)` | Reset all log properties |
| `function log_string  (logptr) str` | Serialize a log into a string |
| `function log_json (logptr) str` | Serialize the log into a JSON string |

Lua libraries included:
- package
- table
- io
- os
- string
- bit32
- math
- debug

## Parser
For now, the parser expects logs to be in the following format:
```
YYYY-MM-dd hh:mm:ss	LEVEL	[Thread]	Class	key: value...
```
More parsers will be added in the future

## Use cases:
- Local aggregation of data
- Dynamic down-sampling
- Event/Error notification
- Log validation and/or sanitization

## Build
If you do not have librdkafka (v0.11.1) installed on your system and if you have docker installed, you can use `make static` to compile and statically link a logd executable without needing to install any dependencies.
