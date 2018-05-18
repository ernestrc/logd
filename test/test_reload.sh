#!/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pipe=$DIR/reload_pipe.pipe
script=$DIR/reload_fixture.lua
out=$DIR/reload_out.log
LOGD_EXEC=$DIR/../bin/logd
PID=

function finish {
	CODE=$?
	rm -f $script
	rm -f $out
	rm -f $pipe
	kill $PID
	exit $CODE;
}

# trap finish EXIT

function makescript() {
	echo '' > $script
	cat >$script << EOF
local logd = require("logd")
function logd.on_log(logptr)
end
print('load')
EOF
}

function updatescript() {
	echo '' > $script
	cat >$script << EOF
local logd = require("logd")
function logd.on_log(logptr)
end
print('reload')
EOF
}

function test_out {
	OUT=$(cat $out)

	if [ "$OUT" != "$1" ]; then
		echo "expected '$1' but found '$OUT'"
		exit 1;
	fi
}

touch $out
mkfifo $pipe; echo

makescript
PID=`$LOGD_EXEC $script > $out & echo $!`
sleep 1
test_out "load"

updatescript
echo '' > $out
kill -s 10 $PID
sleep 1
test_out "reload"

exit 0
