#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/signal.in"
SCRIPT="$DIR/signal.lua"
OUT="$DIR/signal.out"
ERR="$DIR/signal.err"
PID=

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $ERR
	rm -f $IN
	exit $CODE;
}

trap finish EXIT

function makescript() {
	truncate -s 0 $SCRIPT
	cat >$SCRIPT << EOF
local logd = require('logd')
local uv = require('uv')

local timer = uv.new_timer()
uv.timer_start(timer, 1000000, 0, function ()
	uv.timer_stop(timer)
	uv.close(timer)
end)

function logd.on_log(logptr)
	io.write("log")
	io.flush()
end
function logd.on_exit(code, reason)
	-- check that we can still operate on the timer
	uv.close(timer)
	io.write(code)
	io.write(reason)
	io.flush()
end
EOF
}

touch $OUT
touch $IN
makescript
$LOGD_EXEC $SCRIPT -f $IN 2> $ERR 1> $OUT & 
PID=$!
sleep $TESTS_SLEEP

pushdata
assert_file_content "loglog" $OUT

pushdata
assert_file_content "loglogloglog" $OUT

kill -s 2 $PID
sleep $TESTS_SLEEP

if [[ "Darwin" == $(uname) ]]; then
	assert_file_content "loglogloglog1received signal Interrupt: 2" $OUT
else
	assert_file_content "loglogloglog1received signal Interrupt" $OUT
fi

exit 0
