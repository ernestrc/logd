--
-- This example makes full use of the provided builtins to filter and manipulate the logs.
-- First install all the luvit dependencies via `lit install`.
-- This can suplied to the logd executable: logd examples/summary.lua -f /var/log/mylog.log
--
local logd = require("logd")
local https = require('https')
local timer = require('timer')

local tick = 100
local logs = 0
local errors = 0

timer.setTimeout(tick, function ()
	logd.print({ level = 'INFO', msg = "timer triggered" })
end)

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

-- example usage of lit-installed luvit dependency
https.get('https://unstable.build/dummy-get', function(res)
	logd.print(res)
end)
