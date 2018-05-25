TARGET = ../bin

TESTS=$(wildcard test_*.c)
LUA_TESTS=$(wildcard test_*.lua)
INT_TESTS=$(wildcard test_*.sh)
FUZZERS=$(wildcard fuzz_*.c)
TARGET_TESTS = $(addprefix $(TARGET)/,$(patsubst %.c,%,$(TESTS)))
TARGET_FUZZERS = $(addprefix $(TARGET)/,$(patsubst %.c,%,$(FUZZERS)))
TARGET_TPROFILES = $(addprefix $(TARGET)/,$(patsubst %.c,%.profraw,$(TESTS))) $(addprefix $(TARGET)/,$(patsubst %.lua,%.lua.profraw,$(LUA_TESTS)))
TARGET_FPROFILES = $(addprefix $(TARGET)/,$(patsubst %.c,%.profraw,$(FUZZERS)))
TESTPROFDATA=$(TARGET)/tests.profdata
FUZZPROFDATA=$(TARGET)/fuzz.profdata

LIB = ../lib/liblogd.a
SO = ../lib/logd.so
LINK_SO = logd.so
EXEC = $(TARGET)/logd

# a regular lua interpreter should suffice but
# we compile a test runner with instrumentation so lua tests are 
# considered for code coverage
LUNIT = lunit
LUNIT_RUNNER_SRC=$(shell pwd)/$(LUNIT)/extra/lunit.sh
LUNIT_RUNNER = $(shell pwd)/$(TARGET)/lunit
TARGET_LUAJIT= $(TARGET)/luajit
LUNIT_FLAGS ?= --interpreter $(TARGET_LUAJIT)
LUA_PATH=./?.lua;./lunit/lua/?.lua;

LDFLAGS ?= -lm -ldl
FZFLAGS ?= -max_total_time=60 -timeout=30 -use_value_profile=1

.PHONY: clean test fuzz coverage full-coverage fuzz-coverage purge

test: unit_test $(LUNIT) $(LUNIT_RUNNER) $(LINK_SO) lua_test int_test

$(LINK_SO):
	@ ln -s $(SO) $(LINK_SO)

unit_test: $(TARGET_TESTS)
	@ for f in $^; do echo "  TEST	$$f" && LLVM_PROFILE_FILE="$$f.profraw" ./$$f --silent; done

int_test: $(INT_TESTS)
	@ for f in $^; do echo "  TEST	$$f" && LUA_PATH="$(LUA_PATH)" ./$$f 2>&1; done

lua_test: $(LUA_TESTS)
	@ for f in $^; do echo "  TEST	$$f" && LUA_PATH="$(LUA_PATH)" LLVM_PROFILE_FILE="$(TARGET)/$$f.profraw" $(LUNIT_RUNNER) $(LUNIT_FLAGS) $$f 2>/dev/null 1>/dev/null; done

$(TARGET)/test_%: test_%.c $(LIB)
	@ echo "  CC	$@"
	@ $(CC) $(LDFLAGS) $(CFLAGS) -I./ $(addprefix ../src/,$(patsubst test_%.c, %.c, $<)) $< -o $@ $(LIB)

$(LUNIT):
	@ git clone --quiet https://github.com/dcurrie/lunit

$(LUNIT_RUNNER): $(LUNIT_RUNNER_SRC)
	@ echo "  LN	$@"
	@- ln -s $(LUNIT_RUNNER_SRC) $(LUNIT_RUNNER)

$(TESTPROFDATA): $(TARGET_TESTS)
	llvm-profdata merge -sparse $(TARGET_TPROFILES) -o $(TESTPROFDATA)

coverage: $(TESTPROFDATA)
	llvm-cov report -instr-profile=$(TESTPROFDATA) $(addprefix -object ,$(TARGET_TESTS))

full-coverage: $(TESTPROFDATA) BROWSER
	llvm-cov show -format=html -instr-profile=$(TESTPROFDATA) $(addprefix -object ,$(TARGET_TESTS)) > $(TARGET)/coverage.html
	@$(BROWSER) $(shell pwd)/$(TARGET)/coverage.html

fuzz: $(TARGET_FUZZERS)
	@ for f in $^; do echo "  FZ	$$f" && mkdir -p $$f.corpus && LLVM_PROFILE_FILE="$(TARGET)/$$f.profraw" ./$$f $$f.corpus $(FZFLAGS) -dict=./fuzz.dict; done; wait

# clang version >= 6.0.0
$(TARGET)/fuzz_%: fuzz_%.c $(LIB)
	@ $(CC) $(LDFLAGS) $(CFLAGS) -fsanitize=fuzzer $(addprefix ../src/,$(patsubst fuzz_%.c, %.c, $<)) $< -o $@ $(LIB)

$(FUZZPROFDATA): $(TARGET_FUZZERS)
	llvm-profdata merge -sparse $(TARGET_FPROFILES) -o $(FUZZPROFDATA)

fuzz-coverage: $(FUZZPROFDATA)
	llvm-cov report -instr-profile=$(FUZZPROFDATA) $(addprefix -object ,$(TARGET_FUZZERS))

fuzz-full-coverage: $(FUZZPROFDATA) BROWSER
	llvm-cov show -format=html -instr-profile=$(FUZZPROFDATA) $(addprefix -object ,$(TARGET_FUZZERS)) > $(TARGET)/fuzz-coverage.html
	@$(BROWSER) $(shell pwd)/$(TARGET)/fuzz-coverage.html

clean:
	@rm -f $(TARGET_TESTS) $(TARGET_FUZZERS) $(FUZZPROFDATA) $(TESTPROFDATA) $(TARGET_TPROFILES) $(TARGET_FPROFILES) $(LINK_SO) $(LUNIT_RUNNER)

BROWSER:
ifndef BROWSER
	$(error BROWSER env variable is undefined)
endif

purge: clean
	@rm -rf $(LUNIT)
