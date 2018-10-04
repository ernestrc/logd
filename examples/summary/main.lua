--
-- This example makes full use of the provided builtins to filter and manipulate the logs.
--
local logd = require("logd")

local logs = 0
local errors = 0

function logd.on_log(logptr)
	logs = logs + 1

	local date = logd.log_get(logptr, "date")
	local time = logd.log_get(logptr, "time")

	logd.print(string.format('scanned log with date %s and time %s', date, time))
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

function logd.on_exit(code, reason)
	logd.print({
		reason_code = code,
		reason = reason,
		level = 'INFO',
		msg = string.format('scanned %d logs and found %d errors', logs, errors),
	})
end
