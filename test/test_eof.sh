#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/eof.in"
SCRIPT="$DIR/eof.lua"
OUT="$DIR/eof.out"
ERR="$DIR/eof.err"
LOGD_EXEC="$DIR/../bin/logd"

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT $OUT $ERR $IN
	exit $CODE;
}

trap finish EXIT

touch $OUT
touch $ERR
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
function logd.on_exit(code, reason)
	assert(code == 2);
	assert(string.match(reason, 'EOF'));
	io.write(counter)
	io.flush()
end
EOF

cat $IN | $LOGD_EXEC $SCRIPT 2> $ERR 1> $OUT
assert_file_content "2" $OUT

exit 0
