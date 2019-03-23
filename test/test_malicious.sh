#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/malicious.in"
SCRIPT="$DIR/malicious.lua"
OUT="$DIR/malicious.out"
ERR="$DIR/malicious.err"

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT $OUT $IN $ERR
	exit $CODE;
}

trap finish EXIT

touch $OUT
touch $IN

echo -n "2018-05-12 12:51:28 ERROR	[thread1]	clazz	callType: " > $IN
dd if=/dev/urandom count=$(($(( $LOGD_BUF_MAX_CAP / 512 )) * 2)) >> $IN 2>/dev/null
if [ $? -ne 0 ]; then
	echo "could not create test file"
	exit 1
fi

$SED -i ':a;N;$!ba;s/\n/***************/g' $IN

echo "" >> $IN
echo "2018-05-12 12:51:28 ERROR	[thread1]	clazz	callType: so: cool  " >> $IN
echo "2018-05-12 12:51:28 ERROR	[thread1]	clazz	callType: so: cool  " >> $IN

cat >$SCRIPT << EOF
local logd = require("logd")
local errors = 0
local logs = 0
function logd.on_log(logptr)
	logs = logs + 1
end
function logd.on_error(error, logptr, at)
	errors = errors + 1
	local error_expected = "log line was skipped because it is more than $LOGD_BUF_MAX_CAP bytes"
	local at_expected = ""
	assert(error == error_expected, string.format("error was '%s' instead of '%s'", error, error_expected))
	assert(at == at_expected, string.format("at was '%s' instead of '%s'", at, at_expected))
end
function logd.on_exit()
	assert(errors == 1, string.format("expected 1 errors but found %d", errors))
	assert(logs == 2, string.format("expected 2 logs but found %d", logs))
end
EOF

invoke_exec
if [ $? -ne 0 ]; then
	cat $OUT
	cat $ERR
	exit 1
fi

exit 0
