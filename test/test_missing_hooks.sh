#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/missing.in"
SCRIPT="$DIR/missing.lua"
OUT="$DIR/missing.out"
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
EOF

cat >$SCRIPT << EOF
local logd = require("logd")
-- nothing here
EOF

cat $IN | $LOGD_EXEC $SCRIPT 2>> $OUT 1>> $OUT
if [ $? -ne 1 ]; then
	echo "should exit with staus non 0"
	cat $OUT
	exit 1
fi

exit 0
