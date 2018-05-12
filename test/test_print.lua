local lunit = require('lunit')
local logd = require('logd')
local string = string

module("test_print", lunit.testcase, package.seeall)

function trim_ts(str)
	local sample_ts = "2018-05-12 12:52:28"
	return string.sub(str, string.len(sample_ts) + 2, string.len(str))
end

function assert_equal_log(input, output, env)
	logd.print(input)
	lunit.assert_equal(trim_ts(output), trim_ts(env.passed_str))
end

function test_logd_print()
	local cases = {
		{
			output = "2018-05-12 12:52:28 DEBUG	[main]	-	it: works, ",
			input = {
				it = "works",
			},
		},
		{
			output = "2018-05-12 12:52:28 DEBUG	[main]	-	letsee: it: works, ",
			input = {
				it = "works",
				callType = "letsee"
			},
		},
		{
			output = "2018-05-12 12:52:28 ERROR	[main]	-	a: b, ",
			input = {
				level = "ERROR",
				a = "b"
			},
		},
		{
			output = "2018-05-12 12:52:28 INFO	[hello]	hello	",
			input = {
				level = "INFO",
				thread = "hello",
				class = "hello",
			},
		},
		-- TODO {
		-- TODO 	output = "2018-05-12 12:52:28 DEBUG	[main]	-	sup",
		-- TODO 	input = "sup",
		-- TODO }
	}

	for n, c in pairs(cases) do
		local new_env = {unpack(_G)}
		new_env.print = function(str)
			new_env.passed_str = str
		end
		setfenv(0, new_env)
		assert_equal_log(c.input, c.output, new_env)
	end
end
