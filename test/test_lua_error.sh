#!/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/errors.in"
SCRIPT="$DIR/errors.lua"
OUT="$DIR/errors.out"
LOGD_EXEC="$DIR/../bin/logd"

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $IN
	exit $CODE;
}

trap finish EXIT

touch $OUT
touch $IN

cat >$IN << EOF
2018-05-12 12:51:28 ERROR	[thread1]	clazz	a: A, 
EOF

cat >$SCRIPT << EOF
local logd = require("logd")
local uv = require("uv")

local function setTimeout(delay, callback, ...)
  local timer = uv.new_timer()
  local args = {...}
  uv.timer_start(timer, delay, 0, function ()
    uv.timer_stop(timer)
    uv.close(timer)
    callback(unpack(args))
  end)
  return timer
end

function logd.on_log(logptr)
end
function logd.on_eof()
	setTimeout(100, function()
		error('legowelt: dark days')
	end)
	setTimeout(200, function()
		print('warning: getting stuck: consider killing test')
	end)
	setTimeout(100000, function()
		-- we should not wait for this
	end)
end
EOF

cat $IN | $LOGD_EXEC $SCRIPT 2> $OUT 1> $OUT
if [ $? -eq 0 ]; then
	cat $OUT
	exit 1
fi

if [ "$(cat $OUT | grep 'legowelt')" == "" ]; then
	cat $OUT
	exit 1
fi

exit 0
