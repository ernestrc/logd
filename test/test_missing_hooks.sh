#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IN="$DIR/missing.in"
SCRIPT="$DIR/missing.lua"
OUT="$DIR/missing.out"
ERR="$DIR/missing.err"

source $DIR/helper.sh

function finish {
	CODE=$?
	rm -f $SCRIPT $IN $OUT $ERR
	exit $CODE;
}

trap finish EXIT

touch $OUT $IN $ERR

cat >$IN << EOF
2018-05-12 12:51:28 ERROR	[thread1]	clazz	a: A, 
EOF

cat >$SCRIPT << EOF
local logd = require("logd")
-- nothing here
EOF

invoke_exec
if [ $? -ne 1 ]; then
	echo "should exit with staus non 0"
	cat $OUT $ERR
	exit 1
fi

exit 0
