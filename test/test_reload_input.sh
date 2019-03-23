#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/reload_input.in"
IN_MOVED="$DIR/reload_input_moved.in"
SCRIPT="$DIR/reload_input.lua"
OUT="$DIR/reload_input.out"
ERR="$DIR/reload_input.err"
PID=
WRITER_PID=0
WRITER_PID2=0
SIGUSR2=12

if [[ "Darwin" == $(uname) ]]; then
	SIGUSR2=31
fi

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $ERR
	rm -f $IN
	rm -f $IN_MOVED
	kill $PID
	if [ $WRITER_PID -ne 0 ]; then
		kill $WRITER_PID
	fi
	if [ $WRITER_PID2 -ne 0 ]; then
		kill $WRITER_PID2
	fi
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

function makepipe() {
	mkfifo $IN 2> /dev/null 1> /dev/null
	while sleep 1; do :; done >$IN &
}

touch $OUT
makepipe
WRITER_PID=$!
makescript
$LOGD_EXEC $SCRIPT -f $IN 2> $ERR 1> $OUT & 
PID=$!
sleep $TESTS_SLEEP

pushdata
assert_file_content "loglog" $OUT

# move input file
mv $IN $IN_MOVED
makepipe
WRITER_PID2=$!

# send SIGUSR2 to re-open pipe
kill -s $SIGUSR2 $PID

pushdata
assert_file_content "loglogloglog" $OUT

exit 0
