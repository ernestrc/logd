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

touch $OUT
touch $IN

cat >$IN << EOF
2018-05-12 12:52:28 ERROR	[thread]	clazzA	a: A
2018-05-12 12:51:22 ERROR	[thread]	clazzB	callType: b: B
EOF

cat >$SCRIPT << EOF
local logd = require("logd")
local counter = 0
function logd.on_log(logptr)
	counter = counter + 1
end
function logd.on_eof()
	io.write(counter)
	io.flush()
end
EOF

$LOGD_EXEC $SCRIPT -f $IN 2> /dev/null 1> $OUT
assert_file_content "2" $OUT

exit 0
