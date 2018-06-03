--
-- This example makes full use of the provided builtins to filter and manipulate the logs.
-- This can suplied to the logd executable: logd examples/summary.lua -f /var/log/mylog.log
--
local logd = require("logd")
local compat = require('compat')

local tick = 100
local logs = 0
local errors = 0

function logd.on_tick ()
	tick = tick * 2
	logd.config_set("tick", tick)
	logd.print({ level = 'INFO', next_tick = tick, msg = "triggered!" })
end

function logd.on_log(logptr)
	logs = logs + 1
end

function logd.on_error(msg, logptr, at)
	errors = errors + 1
	logd.print({
		level = 'ERROR',
		err = msg,
		at = at,
		partial = logd.to_str(logptr),
	})
end

function logd.on_eof()
	logd.print({ 
		level = 'INFO', 
		msg = string.format('parsed %d logs and found %d errors parsing', logs, errors),
	})
end

-- load backwards compatibility layer with logd-go
compat.load(logd)

-- example usage of legacy "config_set" builtin
logd.config_set("tick", tick)
