--
-- This example makes full use of the provided builtins to filter and manipulate the logs.
-- Logs are printed back to stdout.
-- This can suplied to the logd executable: logd -R examples/summary.lua -f /var/log/mylog.log
--
local logd = require("logd")
local os = require("os")
local uv = require("uv")
local tick = 100
local counter = 0

-- local function set_interval(callback, interval)
--   local timer = uv.new_timer()
--   local function ontimeout()
--     callback(timer)
--   end
--   uv.timer_start(timer, interval, interval, ontimeout)
--   return timer
-- end
-- 
-- set_interval(function ()
-- 	print('tick')
-- end, 1000)

-- TODO function logd.on_tick ()
-- TODO 	tick = tick * 2
-- TODO 	logd.config_set("tick", tick)
-- TODO 
-- TODO 	logd.debug({ next_tick = tick, msg = "triggered!" })
-- TODO end

function logd.on_log(logptr)
	-- TODO logd.debug(string.format("processed log: %s", logd.log_string(logptr)))
	counter = counter + 1
	-- print(counter)
end

-- TODO function logd.on_signal(signal)
-- TODO 	logd.debug({ msg = string.format("My Lua script received signal: %s", signal) })
-- TODO 
-- TODO 	if signal == "SIGUSR1" then
-- TODO 		logd.debug({ msg = "realoading after this function returns.." })
-- TODO 	else
-- TODO 		os.exit(1)
-- TODO 	end
-- TODO end

-- example usage of "config_set" builtin
-- TODO logd.config_set("tick", tick)
