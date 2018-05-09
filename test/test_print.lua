local logd = require("logd")
local passed_str

_G.print = function(str)
	passed_str = str
end

function test_print()
	logd.print({
		it = "works"
	})
	assert(passed_str == "", "not the same")
end
