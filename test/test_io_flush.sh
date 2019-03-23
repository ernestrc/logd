#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/flush.in"
SCRIPT="$DIR/flush.lua"
OUT1="$DIR/flush1.out"
OUT="$DIR/flush.out"
ERR="$DIR/flush2.err"

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT $OUT $OUT1 $ERR $IN
	exit $CODE;
}

function print_files {
	echo "----------------"
	cat $OUT1
	echo "----------------"
	cat $ERR
	echo "----------------"
}

trap finish EXIT

touch $OUT1 $ERR $OUT $IN

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

function logd.on_exit()
	setTimeout(100, function()
		logd.print({ legowelt = 'dark days' })
	end)
	setTimeout(200, function()
		print('mind games')
	end)
	setTimeout(1000, function()
		-- now we are done
	end)
end

io.output('$OUT1')
EOF

invoke_exec 
retval=$?
if [ $retval -ne 0 ]; then
	echo "script exit with non-zero status code: $retval"
	print_files
	exit 1
fi

if [ "$(cat $OUT1 | grep 'legowelt')" == "" ]; then
	echo "io.output file did not contain expected data"
	print_files
	exit 1
fi

if [ "$(cat $ERR $OUT | grep 'mind games')" == "" ]; then
	echo "stdout redirect to file did not contain expected data"
	print_files
	exit 1
fi

exit 0
