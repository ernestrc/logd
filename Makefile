.PHONY: test analysis full-analysis fuzz format tags coverage full-coverage fuzz-coverage fuzz-full-coverage debug purge

TEST_SLAB_CAP=30
TEST_BUF_MAX_CAP=1000000
TEST_BUF_INIT_CAP=64000
TEST_CFLAGS=-pthread -Wall -Wno-unused-function -Werror -fsanitize=undefined -fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping -std=c11 -ggdb -DLOGD_DEBUG -D_GNU_SOURCE -DLOGD_SLAB_CAP=$(TEST_SLAB_CAP) -DLOGD_BUF_MAX_CAP=$(TEST_BUF_MAX_CAP) -DLOGD_INIT_BUF_CAP=$(TEST_BUF_INIT_CAP)

PWD=$(shell pwd)
BIN=$(PWD)/bin
INCLUDE=$(PWD)/include
LIB=$(PWD)/lib
TEST=$(PWD)/test
SRC=$(PWD)/src
DEPS=$(PWD)/deps

default: build

prepare:
	@ git submodule update --init --recursive
	@ mkdir -p $(LIB)
	@ mkdir -p $(BIN)
	@ mkdir -p $(INCLUDE)

build-deps: prepare
	@ cd $(DEPS) && $(MAKE)

build: build-deps
	@ cd $(SRC) && $(MAKE)

analysis: clean
	@ scan-build $(MAKE)

# run clang-analyzer on deps + src
full-analysis: purge
	@ scan-build $(MAKE) -s

debug: export CFLAGS = $(TEST_CFLAGS)
debug: build-deps
	@ cd $(SRC) && $(MAKE) build

test: export SLAB_CAP=$(TEST_SLAB_CAP)
test: export BUF_MAX_CAP=$(TEST_BUF_MAX_CAP)
test: export CFLAGS = $(TEST_CFLAGS)
test: debug
	@ cd $(TEST) && $(MAKE) $@

fuzz: clean debug
	@ cd $(TEST) && $(MAKE) $@

coverage:
	cd $(TEST) && $(MAKE) $@

full-coverage:
	cd $(TEST) && $(MAKE) $@

fuzz-coverage:
	cd $(TEST) && $(MAKE) $@

fuzz-full-coverage:
	cd $(TEST) && $(MAKE) $@

format:
	@ find $(SRC) -name \*.h -o -name \*.c | xargs clang-format -i
	@ find $(TEST) -name \*.h -o -name \*.c | xargs clang-format -i

tags:
	@ ctags -R

clean:
	@ rm -rf $(BIN) $(LIB) $(INCLUDE)
	@ cd $(SRC) && $(MAKE) $@
	@ cd $(TEST) && $(MAKE) $@
	@ rm -f *.profraw

purge: prepare clean
	@ cd $(DEPS) && $(MAKE) clean
	@ git submodule foreach --recursive git clean -xfd
	@ git submodule foreach --recursive git reset --hard
	@ rm -f tags
