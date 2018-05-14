local lunit = require('lunit')
local helper = {}

helper.trim_ts = function(str)
	local sample_ts = "2018-05-12 12:52:28"
	return string.sub(str, string.len(sample_ts) + 2, string.len(str))
end

helper.assert_equal_log = function(a, b)
	lunit.assert_equal(helper.trim_ts(a), helper.trim_ts(b))
end

return helper
