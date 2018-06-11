.PHONY: analysis full-analysis format tags purge

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
