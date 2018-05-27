local uv = require('uv')

local compat = {}
local __t

local function clear_timeout(timer)
  uv.timer_stop(timer)
  uv.close(timer)
end

local function set_timeout(callback, timeout)
  local timer = uv.new_timer()
  local function ontimeout()
	clear_timeout(timer)
    callback(timer)
  end
  uv.timer_start(timer, timeout, 0, ontimeout)
  return timer
end

local function set_interval(callback, interval)
  local timer = uv.new_timer()
  local function ontimeout()
    callback(timer)
  end
  uv.timer_start(timer, interval, interval, ontimeout)
  return timer
end

local clear_interval = clear_timeout

function compat.load(logd)
	prev_on_eof = logd.on_eof
	logd.on_eof = function()
		if __t ~= nil then
			clear_interval(__t)
		end
		if prev_on_eof ~= nil then
			prev_on_eof()
		end
	end
	logd.config_set = function(key, value)
		if (key == "tick") then
			if __t ~= nil then
				clear_interval(__t)
			end
			__t = set_interval(logd.on_tick, value)
		end
	end
end

return compat
