#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/tail_input.in"
IN_MOVED="$DIR/tail_input_moved.in"
SCRIPT="$DIR/tail_input.lua"
OUT="$DIR/tail_input.out"
ERR="$DIR/tail_input.err"
LOGD_EXEC="$DIR/../bin/logd"
PID=

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $ERR
	rm -f $IN
	rm -f $IN_MOVED
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
	io.flush()
end
EOF
}

function pushdata() {
	cat >>$IN << EOF
2018-05-12 12:51:28 ERROR	[thread1]	clazz	a: A, 
2018-05-12 12:52:22 WARN	[thread2]	clazz	callType: b: B
EOF
}

touch $OUT
touch $IN
makescript
$LOGD_EXEC $SCRIPT -f $IN 2> $ERR 1> $OUT & 
PID=$!
sleep 1

pushdata
assert_file_content "loglog" $OUT

# move input file
mv $IN $IN_MOVED
touch $IN

# send SIGUSR2 to re-open pipe
kill -s 12 $PID
 
pushdata
assert_file_content "loglogloglog" $OUT

exit 0
