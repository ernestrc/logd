#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/shutdown.in"
SCRIPT="$DIR/shutdown.lua"
OUT="$DIR/shutdown.out"
ERR="$DIR/shutdown.err"
LOGD_EXEC="$DIR/../bin/logd"
PID=
WRITER_PID=0

source $DIR/helper.sh

function print_files {
	echo "----------------"
	cat $OUT
	echo "----------------"
	cat $ERR
	echo "----------------"
}

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $ERR
	rm -f $IN
	if [ $WRITER_PID -ne 0 ]; then
		kill $WRITER_PID
	fi
	exit $CODE;
}

trap finish EXIT

touch $OUT
touch $ERR

mkfifo $IN 2> /dev/null 1> /dev/null

while sleep 1; do :; done >$IN &
WRITER_PID=$!

# we use full i/o buffering so if we write some data
# and do not flush, only if we are gracefully
# shutting down process, the data should be flushed.
cat >$SCRIPT << EOF
local logd = require('logd')

function logd.on_log(logptr)
end

io.output('$OUT')
io.output():setvbuf('full')
io.write('such grace')
EOF

cat $IN | $LOGD_EXEC $SCRIPT 1>> $ERR 2> $ERR &
PID=$!

sleep $TESTS_SLEEP
kill -s SIGINT $PID

if [ "$(cat $OUT | grep 'such grace')" == "" ]; then
	echo "we did not gracefully shut down process:"
	print_files
	exit 1
fi

exit 0
