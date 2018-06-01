local lunit = require('lunit')
local fixture = require('fixture')
local helper = require('helper')
local logd = require('logd')

module("test_print", lunit.testcase, package.seeall)

function assert_equal_log(input, output, env)
	logd.print(input)
	lunit.assert_equal(helper.trim_ts(output), helper.trim_ts(env.passed_str))
end

function mock_env()
	local new_env = {unpack(_G)}
	new_env.io = {
		write = function(str)
			if str ~= "\n" then
				new_env.passed_str = str
			end
		end
	}
	setfenv(0, new_env)
	return new_env
end

function test_logd_print()
	-- test logd.print(table|str)
	for n, c in pairs(fixture.cases) do
		local new_env = mock_env()
		assert_equal_log(c.input, c.output, new_env)
	end
	-- test logd.print(logptr)
	for n, c in pairs(fixture.table_cases) do
		local new_env = mock_env()
		assert_equal_log(logd.to_logptr(c.input), c.output, new_env)
	end
end
