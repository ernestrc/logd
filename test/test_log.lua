local lunit = require('lunit')
local fixture = require('fixture')
local helper = require('helper')
local logd = require('logd')

module("test_log", lunit.testcase, package.seeall)

function test_logd_log_ptr_str()
	for i, c in pairs(fixture.table_cases) do
		local ptr = logd.to_logptr(c.input)
		helper.assert_equal_log(logd.to_str(ptr), c.output)
	end
end
