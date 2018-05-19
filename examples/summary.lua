--
-- This example makes full use of the provided builtins to filter and manipulate the logs.
-- Logs are printed back to stdout.
-- This can suplied to the logd executable: logd -R examples/summary.lua -f /var/log/mylog.log
--
local logd = require("logd")
local compat = require('compat')
local tick = 100
local counter = 0

function logd.on_tick ()
	tick = tick * 2
	logd.config_set("tick", tick)

	logd.print({ level = 'INFO', next_tick = tick, msg = "triggered!" })
end

function logd.on_log(logptr)
	counter = counter + 1
	logd.print({
		msg = "processed new log",
		counter = counter,
	})
end

-- load backwards compatibility layer with logd-go
compat.load(logd)

-- example usage of "config_set" builtin
logd.config_set("tick", tick)
