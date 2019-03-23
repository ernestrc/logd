#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/safe_logptr.in"
SCRIPT="$DIR/safe_logptr.lua"
OUT="$DIR/safe_logptr.out"
ERR="$DIR/safe_logptr.err"

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT $OUT $ERR $IN
	exit $CODE;
}

trap finish EXIT

function makescript() {
	truncate -s 0 $SCRIPT
	cat >$SCRIPT << EOF
local logd = require("logd")
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
	setTimeout(0, function()
		local class = logd.log_get(logptr, "class")
		assert(false, "NOPE")
	end)
end
EOF
}

function pushdata() {
	cat >>$IN << EOF
2018-05-12 12:51:28 ERROR	[thread1]	clazz	a: A, 
EOF
	sync $IN
	sleep $TESTS_SLEEP
}

touch $OUT $ERR $IN
makescript
pushdata
invoke_exec
# TODO exit code should be non-zero
# RET=$?
# if [ $? -eq 0 ]; then
# 	echo "------------"
# 	cat $OUT
# 	echo "------------"
# 	cat $ERR
# 	echo "------------"
# 	exit 1
# fi

assert_file_contains "it is not safe to use a logptr" $ERR

exit 0
