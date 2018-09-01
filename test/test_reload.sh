#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/reload.in"
SCRIPT="$DIR/reload.lua"
OUT="$DIR/reload.out"
ERR="$DIR/reload.err"
LOGD_EXEC="$DIR/../bin/logd"
PID=
WRITER_PID=0

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $ERR
	rm -f $IN
	kill $PID
	if [ $WRITER_PID -ne 0 ]; then
		kill $WRITER_PID
	fi
	exit $CODE;
}

trap finish EXIT

function makescript() {
	truncate -s 0 $SCRIPT
	cat >$SCRIPT << EOF
local logd = require("logd")
function logd.on_log(logptr)
end
io.write("load")
io.flush()
EOF
}

function updatescript() {
	truncate -s 0 $SCRIPT
	cat >$SCRIPT << EOF
local logd = require("logd")
function logd.on_log(logptr)
end
io.write("reload")
io.flush()
EOF
}

touch $OUT
mkfifo $IN 2> /dev/null 1> /dev/null

while sleep 1; do :; done >$IN &
WRITER_PID=$!

makescript
$LOGD_EXEC $SCRIPT -f $IN 2> $ERR 1> $OUT & 
PID=$!
sleep $TESTS_SLEEP
assert_file_content "load" $OUT

updatescript
truncate -s 0 $OUT
kill -s 10 $PID
sleep $TESTS_SLEEP
assert_file_content "reload" $OUT

exit 0
