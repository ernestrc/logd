#!/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/malicious.in"
SCRIPT="$DIR/malicious.lua"
OUT="$DIR/malicious.out"
LOGD_EXEC="$DIR/../bin/logd"

if [ "$BUF_MAX_CAP" == "" ]; then
	BUF_MAX_CAP=1000
	echo "WARN: using BUF_MAX_CAP=$BUF_MAX_CAP; note that this could be out of sync with Makefile. Use make test instead."
fi

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

echo -n "2018-05-12 12:51:28 ERROR	[thread1]	clazz	callType: " > $IN
dd if=/dev/urandom count=$(($(( $BUF_MAX_CAP / 512 )) + 10)) >> $IN 2>/dev/null
if [ $? -ne 0 ]; then
	echo "could not create test file"
	exit 1
fi

sed -i ':a;N;$!ba;s/\n/***************/g' $IN

echo "" >> $IN

cat >$SCRIPT << EOF
local logd = require("logd")
local errors = 0
function logd.on_log(logptr)
	error("should not have been called")
end
function logd.on_eof()
	assert(errors == 1)
end
function logd.on_error(error, logptr, remaining)
	errors = errors + 1
	local error_expected = "log line was skipped because it is more than $BUF_MAX_CAP bytes"
	local remaining_expected = ""
	assert(error == error_expected, string.format("error was '%s' instead of '%s'", error, error_expected))
	assert(remaining == remaining_expected, string.format("remaining was '%s' instead of '%s'", remaining, remaining_expected))
end
EOF

$LOGD_EXEC $SCRIPT -f $IN 2> $OUT 1> $OUT
if [ $? -ne 0 ]; then
	cat $OUT
	exit 1
fi

exit 0
