#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/errors.in"
SCRIPT="$DIR/errors.lua"
OUT="$DIR/errors.out"
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
2018-05-12 12:51:28 ERROR	[thread1]	clazz	a: A, 

2018-05-12 12:53:22 INFO	[thread3]	clazz	callType: c: C, 
2018-05-12 GARBAGE DEBUG	[thread4]	clazz	callType: b: ,
2018-05-12 12:55:22 TRACE	[thread5]	clazz	tooManyProps: b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, : c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, b: c: C, 
2018-05-12 12:53:22 INFO	[thread3]	clazz	callType: c: C, 
EOF

cat >$SCRIPT << EOF
local logd = require("logd")
local logs = 0
local errors = 0
function logd.on_log(logptr)
	logs = logs + 1
end
function logd.on_exit()
	assert(logs == 3)
end
function logd.on_error(error, logptr, at)
	local error_expected
	local at_expected
	errors = errors + 1
	-- should be able to use log_get on logptr
	logd.log_get(logptr, "date")
	if errors == 1 then
		error_expected = "incomplete header"
		at_expected = ""
	elseif errors == 2 then
		error_expected = "invalid date or time in log header"
		at_expected = "GARBAGE DEBUG	[thread4]	clazz	callType: b: ,"
	else
		error_expected = "reached max number of log properties: $LOGD_SLAB_CAP"
	end

	assert(errors < 4)
	assert(error == error_expected, string.format("error was '%s' instead of '%s'", error, error_expected))
	if at_expected ~= nil then
		assert(at == at_expected, string.format("at was '%s' instead of '%s'", at, at_expected))
	end
end
EOF

cat $IN | $LOGD_EXEC $SCRIPT 2>> $OUT 1>> $OUT
if [ $? -ne 0 ]; then
	cat $OUT
	exit 1
fi

exit 0
