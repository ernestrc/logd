local logd = require('logd')
local fixture = {}
fixture.table_cases = {
	{
		output = "2018-05-12 12:52:28 DEBUG	[-]	-	it: works, ",
		input = {
			it = "works",
		},
	},
	{
		output = "2018-05-12 12:52:28 DEBUG	[-]	-	letsee: it: works, ",
		input = {
			it = "works",
			callType = "letsee"
		},
	},
	{
		output = "2018-05-12 12:52:28 ERROR	[-]	-	a: b, ",
		input = {
			level = "ERROR",
			a = "b"
		},
	},
	{
		output = "2018-05-12 12:52:28 INFO	[my  thread ]	hello	",
		input = {
			level = "INFO",
			thread = "my  thread ",
			class = "hello",
		},
	},
	{
		output = "2018-05-12 12:52:28 DEBUG	[-]	-	userdata: <ptr>, tbl: <table>, fun: <func>, boolean: true, thr: <thread>, number: 45330342000000, ",
		input = {
			number = 45330342000000,
			boolean = true,
			void = nil, -- not inserted
			tbl = { my = 'table' },
			thr = coroutine.create(function() end),
			userdata = logd.to_logptr({ key_year = '1714' }),
			fun = function() end,
		},
	},
}

fixture.str_cases = {
	{
		output = "2018-05-12 12:52:28 DEBUG	[-]	-	msg: it: should not, sanitize: input, ",
		input = "it: should not, sanitize: input",
	}
}

fixture.cases = {}

for k,v in pairs(fixture.str_cases) do table.insert(fixture.cases, v) end
for k,v in pairs(fixture.table_cases) do table.insert(fixture.cases, v) end

return fixture
