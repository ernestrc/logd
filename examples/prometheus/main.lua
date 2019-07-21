--
-- This example makes usage of luvit-prometheus module.
-- Before running, make sure you install all the dependencies via `lit install`.
--
local logd = require("logd")
local http = require('http')

-- init prometheus module
local prometheus = require('prometheus').init('logd_example_', {
  log = function(level, msg)
    logd.print({
    	level = level,
    	msg = "Prometheus error: " .. msg
	})
  end
})

-- init prometheus counter
local metrics = prometheus:counter('logs', 'logs processed', {'status'})

-- collect metrics and satisfy request
local function on_metrics_request(req, res)
  logd.print("reporting metrics to prometheus")

  local body = prometheus:collect()
  res:setHeader("Content-Type", "text/plain")
  res:setHeader("Content-Length", #body)
  res:finish(body)
end

local function setup_metrics_server(port)
  if metrics == nil then
  	  logd.print("Something went wrong when setting up Prometheus counter")
  else
      http.createServer(on_metrics_request):listen(port)
      logd.print("Prometheus exporter listening at http://localhost:8080/")
  end
end

-- example outgoing POST
function logd.on_log(logptr)
	metrics:inc(1, {'success'})
end

function logd.on_error(msg, logptr, at)
	metrics:inc(1, {'failure'})
end

setup_metrics_server(8080)
