#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/signal.in"
SCRIPT="$DIR/signal.lua"
OUT="$DIR/signal.out"
ERR="$DIR/signal.err"
LOGD_EXEC="$DIR/../bin/logd"
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
local logd = require("logd")
function logd.on_log(logptr)
	io.write("log")
	io.flush()
end
function logd.on_exit(code, reason)
    io.write(code)
	io.write(reason)
	io.flush()
end
EOF
}

function pushdata() {
	cat >$IN << EOF
2018-05-12 12:51:28 ERROR	[thread1]	clazz	a: A, 
2018-05-12 12:52:22 WARN	[thread2]	clazz	callType: b: B
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

assert_file_content "loglogloglog1received signal Interrupt" $OUT

exit 0
