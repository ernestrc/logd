local lunit = require('lunit')
local fixture = require('fixture')
local helper = require('helper')
local logd = require('logd')

module("test_log", lunit.testcase, package.seeall)

local sample_log = {
	a = "a",
	date = "b",
	thread = "myThread",
	callType = "myCallType",
}

local function assert_sample_log(ptr)
	lunit.assert_equal("a", logd.log_get(ptr, "a"))
	lunit.assert_equal("b", logd.log_get(ptr, "date"))
	lunit.assert_equal("myThread", logd.log_get(ptr, "thread"))
	lunit.assert_equal("myCallType", logd.log_get(ptr, "callType"))
end

function test_logd_log_ptr_str()
	for i, c in pairs(fixture.table_cases) do
		local ptr = logd.to_logptr(c.input)
		helper.assert_equal_log(logd.to_str(ptr), c.output)
	end
end

function test_logd_log_get()
	local ptr = logd.to_logptr(sample_log)
	assert_sample_log(ptr)
end

function test_logd_log_set()
	local ptr = logd.to_logptr({})

	for k,v in pairs(sample_log) do
		logd.log_set(ptr, k, v)
	end
	assert_sample_log(ptr)
end

function test_logd_log_remove()
	local ptr = logd.to_logptr(sample_log)
	for k,v in pairs(sample_log) do
		logd.log_remove(ptr, k)
	end
	for k,v in pairs(sample_log) do
		lunit.assert_equal(nil, logd.log_get(ptr, k))
	end
end

function test_logd_log_reset()
	local ptr = logd.to_logptr(sample_log)
	logd.log_reset(ptr)
	for k,v in pairs(sample_log) do
		lunit.assert_equal(nil, logd.log_get(ptr, k))
	end
end

function test_logd_log_clone()
	local ptr = logd.to_logptr(sample_log)
	local clone = logd.log_clone(ptr)
	assert_sample_log(clone)
end

function test_logd_log_to_table()
	local ptr = logd.to_logptr(sample_log)
	local clone = logd.to_logptr(logd.to_table(ptr))
	assert_sample_log(clone)
end
