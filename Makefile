.PHONY: analysis full-analysis format tags purge test fuzz coverage html-coverage fuzz-coverage fuzz-html-coverage

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
	@ $(MAKE) -C $(DEPS)

build: build-deps
	@ $(MAKE) -C $(SRC)

analysis: clean
	@ scan-build $(MAKE)

# run clang-analyzer on deps + src
full-analysis: purge
	@ scan-build $(MAKE) -s

test: build
	@ $(MAKE) $@ -C $(TEST)

fuzz: build
	@ $(MAKE) $@ -C $(TEST)

coverage:
	@ $(MAKE) $@ -C $(TEST)

html-coverage:
	@ $(MAKE) $@ -C $(TEST)

fuzz-coverage:
	@ $(MAKE) $@ -C $(TEST)

fuzz-html-coverage:
	@ $(MAKE) $@ -C $(TEST)

format:
	@ find $(SRC) -name \*.h -o -name \*.c | xargs clang-format -i
	@ find $(TEST) -name \*.h -o -name \*.c | xargs clang-format -i

tags:
	@ ctags -R

clean:
	@ rm -rf $(BIN) $(LIB) $(INCLUDE)
	@ $(MAKE) $@ -C $(SRC)
	@ $(MAKE) $@ -C $(TEST)
	@ rm -f *.profraw

purge: prepare clean
	@ rm -rf autom4te.cache config.status config.log
	@ $(MAKE) clean -C $(DEPS)
	@ git submodule foreach --recursive git clean -xfd
	@ git submodule foreach --recursive git reset --hard
	@ rm -f tags

# TODO
install:
	@ echo TODO
