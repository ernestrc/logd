--
-- This example makes usage of lit-installed luvit modules.
-- Before running, make sure you install all the dependencies via `lit install`.
--
local logd = require("logd")
local JSON = require('json')
local URL = require('url')
local http = require('http')
local https = require('https')

local tick = 100
local logs = 0
local errors = 0

local function request(opt)
  local req
  if opt.protocol == 'https' then
    opt.secureProtocol = 'TLSv1_1'
    req = https.request(opt)
  else
    req = http.request(opt)
  end

  return req
end

function post_event(url, event, cb)
  local body = JSON.encode(event)
  local opt = URL.parse(url)
  opt.method = 'POST'
  opt.headers = {
    ['Content-Type'] = 'application/json',
    ['Content-Length'] = #body,
  }

  local done
  local req = request(opt)

  req:on('response', function(res)
    if res.statusCode < 200 or res.statusCode >= 300 then
      done(string.format('request failed with status code: %d', res.statusCode))
      res:resume()
      return
    end
    res:on('end', function()
      done(nil)
    end)
    res:resume()
  end)

  req:on('socket', function(socket)
    socket:on('error', function(err)
      done(err)
    end)
  end)

  req:on('error', function(err)
    done(string.format('request error: %s', err))
  end)

  done = function(err)
    cb(err)
    req:removeAllListeners()
    done = function() end
  end

  req:done(body)
end

function logd.on_log(logptr)
	logs = logs + 1

	local event = {}
	event.date = logd.log_get(logptr, 'date')
	event.time = logd.log_get(logptr, 'time')
	event.msg = logd.log_get(logptr, 'msg')

	post_event('https://unstable.build/log', event, function(err)
		if err ~= nil then
			logd.print({
				msg = 'Bummer!',
				err = err,
			})
			return
		end
		logd.print('Yay!')
	end)
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
