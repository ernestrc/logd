#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/file.in"
SCRIPT="$DIR/file.lua"
OUT="$DIR/file.out"
LOGD_EXEC="$DIR/../bin/logd"
PUSH_FILE_ITER=1000

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT
	rm -f $IN
	rm -f $OUT
	exit $CODE;
}

trap finish EXIT

touch $OUT
touch $IN

push_file $1

cat >$SCRIPT << EOF
local logd = require("logd")
local expected = $(( 12 * $PUSH_FILE_ITER ))
local counter = 0
function logd.on_log(logptr)
  local level = logd.log_get(logptr, "level")
  if level == "ERROR" then
	  counter = counter + 1
  end
end
function logd.on_exit()
	assert(counter == expected,
		string.format("expected counter to be %d but found %d", expected, counter))
end
EOF

# stdin
cat $IN | $LOGD_EXEC $SCRIPT 2>> $OUT 1>> $OUT
if [ $? -ne 0 ]; then
	cat $OUT
	echo "error processing file via stdin"
	exit 1
fi

exit 0
