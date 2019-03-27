#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/multif.in"
SCRIPT="$DIR/multif.lua"
OUT="$DIR/multif.out"
ERR="$DIR/multif.err"
PUSH_FILE_ITER=1000
NUM_FILES=3

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT $IN $OUT $ERR
	exit $CODE;
}

trap finish EXIT

touch $OUT $IN $ERR

push_file $1

cat >$SCRIPT << EOF
local logd = require("logd")
local expected = $(( 12 * $PUSH_FILE_ITER * $NUM_FILES ))
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

$LOGB_EXEC $SCRIPT $IN $IN $IN 2> $ERR 1> $OUT
if [ $? -ne 0 ]; then
	cat $OUT $ERR
	echo "error processing multiple files"
	exit 1
fi

exit 0
