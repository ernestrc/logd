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
			thread = "my:\nthread,",
			class = "hello",
		},
	},
}

fixture.str_cases = {
	{
		output = "2018-05-12 12:52:28 DEBUG	[-]	-	msg: it  should  sanitize  input, ",
		input = "it: should, sanitize: input",
	}
}

fixture.cases = {}

for k,v in pairs(fixture.str_cases) do table.insert(fixture.cases, v) end
for k,v in pairs(fixture.table_cases) do table.insert(fixture.cases, v) end

return fixture
