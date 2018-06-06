local luviBundle = require('luvibundle')
local commonBundle = luviBundle.commonBundle


return function(script)
	local dir
	if script:match(".-/.-") then
		dir = string.gsub(script, "(.*/)(.*)", "%1")
	else
		dir = '.'
	end
	local file = string.gsub(script, "(.*/)(.*)", "%2")
	local bundles = { [1] = dir }
	local appArgs = { [1] = "logd" }

	-- for simple use cases, add script dirname to package.path
	package.path = package.path .. string.format(";%s/?.lua", dir)

	return commonBundle(bundles, file, appArgs)
end
