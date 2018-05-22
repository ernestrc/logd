#!/bin/env bash

function assert_file_content {
	local __OUT=$(tr -d '\0' < $2)
	

	if [ "$__OUT" != "$1" ]; then
		echo "expected '$1' but found '$__OUT'"
		exit 1;
	fi
}
