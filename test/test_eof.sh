#!/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/eof.in"
SCRIPT="$DIR/eof.lua"
OUT="$DIR/eof.out"
LOGD_EXEC="$DIR/../bin/logd"

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $OUT
	rm -f $IN
	exit $CODE;
}

trap finish EXIT

function makescript() {
	truncate -s 0 $SCRIPT
	cat >$SCRIPT << EOF
local logd = require("logd")
function logd.on_log(logptr)
end
function logd.on_eof()
	io.write("eof")
	io.flush()
end
EOF
}

touch $OUT
touch $IN

makescript
$LOGD_EXEC $SCRIPT -f $IN 2> /dev/null 1> $OUT
assert_file_content "eof" $OUT

exit 0
