#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/input_retry.in"
SCRIPT="$DIR/input_retry.lua"
OUT="$DIR/input_retry.out"
ERR="$DIR/input_retry.err"
LOGD_EXEC="$DIR/../bin/logd"
PID=

source $DIR/helper.sh
TESTS_SLEEP_MS=$(($TESTS_SLEEP * 1000))

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $ERR
	rm -f $IN
	kill $PID
	exit $CODE;
}

trap finish EXIT

function makescript() {
	truncate -s 0 $SCRIPT
	cat >$SCRIPT << EOF
local logd = require("logd")
function logd.on_log(logptr)
	io.write("log")
	io.flush() -- setvbuf default is line
end
EOF
}

touch $OUT
makescript
# start consuming even when input file is not created yet
$LOGD_EXEC $SCRIPT -r 5 -d $TESTS_SLEEP_MS -f $IN 2> $ERR 1> $OUT &
PID=$!
sleep $TESTS_SLEEP

touch $IN
sleep $TESTS_SLEEP

pushdata
assert_file_content "loglog" $OUT

exit 0
