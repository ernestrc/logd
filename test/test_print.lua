local logd = require "logd"
local passed_str

module("test_print", lunit.testcase, package.seeall)

function setup()
end

function test_table()
	logd.print({
		it = "works",
		callType = "letsee"
	})
	assert_equal("DEBUG	[main]	-	letsee: it: works, ", passed_str)
end
