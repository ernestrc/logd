TARGET = ../bin

TESTS=$(wildcard test_*.c)
FUZZERS=$(wildcard fuzz_*.c)
TARGET_TESTS = $(addprefix $(TARGET)/,$(patsubst %.c,%,$(TESTS)))
TARGET_FUZZERS = $(addprefix $(TARGET)/,$(patsubst %.c,%,$(FUZZERS)))
TARGET_TPROFILES = $(addprefix $(TARGET)/,$(patsubst %.c,%.profraw,$(TESTS)))
TARGET_FPROFILES = $(addprefix $(TARGET)/,$(patsubst %.c,%.profraw,$(FUZZERS)))

LIB = ../lib/liblogd.a

ifndef FUZZER_TOTAL_TIME
FUZZER_TOTAL_TIME=60
endif

LDFLAGS ?= -lm -ldl

TESTPROFDATA=$(TARGET)/tests.profdata
FUZZPROFDATA=$(TARGET)/fuzz.profdata

.PHONY: clean test fuzz coverage full-coverage fuzz-coverage purge

test: $(TARGET_TESTS)
	@ for f in $^; do LLVM_PROFILE_FILE="$$f.profraw" ./$$f; done

$(TARGET)/test_%: test_%.c $(LIB)
	@ echo "  CC	$@"
	@ $(CC) $(LDFLAGS) $(CFLAGS) -I./ $(addprefix ../src/,$(patsubst test_%.c, %.c, $<)) $< -o $@ $(LIB)

$(TESTPROFDATA): $(TARGET_TESTS)
	llvm-profdata merge -sparse $(TARGET_TPROFILES) -o $(TESTPROFDATA)

coverage: $(TESTPROFDATA)
	llvm-cov report -instr-profile=$(TESTPROFDATA) $(addprefix -object ,$(TARGET_TESTS))

full-coverage: $(TESTPROFDATA) BROWSER
	llvm-cov show -format=html -instr-profile=$(TESTPROFDATA) $(addprefix -object ,$(TARGET_TESTS)) > $(TARGET)/coverage.html
	@$(BROWSER) $(shell pwd)/$(TARGET)/coverage.html

fuzz: $(TARGET_FUZZERS)
	@ for f in $^; do mkdir -p $$f.corpus && LLVM_PROFILE_FILE="$$f.profraw" $$f $$f.corpus -max_total_time=$(FUZZER_TOTAL_TIME); done; wait

# clang version >= April 2017 required (tested with 6.0.0 trunk 310227)
$(TARGET)/fuzz_%: fuzz_%.c $(LIB)
	@ echo "  CC	$@"
	@ $(CC) $(LDFLAGS) $(CFLAGS) -fsanitize=fuzzer $(addprefix ../src/,$(patsubst fuzz_%.c, %.c, $<)) $< -o $@ $(LIB)

$(FUZZPROFDATA): $(TARGET_FUZZERS)
	llvm-profdata merge -sparse $(TARGET_FPROFILES) -o $(FUZZPROFDATA)

fuzz-coverage: $(FUZZPROFDATA)
	llvm-cov report -instr-profile=$(FUZZPROFDATA) $(addprefix -object ,$(TARGET_FUZZERS))

fuzz-full-coverage: $(FUZZPROFDATA) BROWSER
	llvm-cov show -format=html -instr-profile=$(FUZZPROFDATA) $(addprefix -object ,$(TARGET_FUZZERS)) > $(TARGET)/fuzz-coverage.html
	@$(BROWSER) $(shell pwd)/$(TARGET)/fuzz-coverage.html

clean:
	@rm -f $(TARGET_TESTS) $(TARGET_FUZZERS) $(FUZZPROFDATA) $(TESTPROFDATA) $(TARGET_TPROFILES) $(TARGET_FPROFILES)

BROWSER:
ifndef BROWSER
	$(error BROWSER env variable is undefined)
endif

purge: clean
