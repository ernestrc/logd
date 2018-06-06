default: src

.PHONY: test analysis full-analysis fuzz format tags coverage full-coverage fuzz-coverage fuzz-full-coverage debug purge
.SILENT:
export CC = clang

TEST_SLAB_CAP=30
TEST_BUF_MAX_CAP=1000000
TEST_BUF_INIT_CAP=64000
TEST_CFLAGS=-pthread -Wall -Wno-unused-function -Werror -fsanitize=undefined -fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping -std=c11 -ggdb -DLOGD_DEBUG -D_GNU_SOURCE -DLOGD_SLAB_CAP=$(TEST_SLAB_CAP) -DLOGD_BUF_MAX_CAP=$(TEST_BUF_MAX_CAP) -DLOGD_INIT_BUF_CAP=$(TEST_BUF_INIT_CAP)
FUZZ_CFLAGS=$(TEST_CFLAGS)

TARGET=./bin
LIB=./lib
TEST=./test
SRC=./src
EXT=./ext

deps:
	@ cd $(EXT) && $(MAKE) $@

prepare:
	@ mkdir -p $(LIB)
	@ mkdir -p $(TARGET)

clean:
	@ rm -rf $(TARGET) $(LIB)
	@ cd $(SRC) && $(MAKE) $@
	@ cd $(TEST) && $(MAKE) $@
	@ rm -f *.profraw

# run clang-analyzer
analysis: clean
	@ scan-build $(MAKE)

# run clang-analyzer on dependencies + src
full-analysis: purge
	@ scan-build $(MAKE) -s

src: prepare deps
	@ cd src && $(MAKE) $@

debug: export CFLAGS = $(TEST_CFLAGS)
debug: prepare deps
	@ cd src && $(MAKE) src


test: export SLAB_CAP=$(TEST_SLAB_CAP)
test: export BUF_MAX_CAP=$(TEST_BUF_MAX_CAP)
test: export CFLAGS = $(TEST_CFLAGS)
test: debug
	@ cd $(TEST) && $(MAKE) $@

fuzz: export SLAB_CAP=$(TEST_SLAB_CAP)
fuzz: export BUF_MAX_CAP=$(TEST_BUF_MAX_CAP)
fuzz: export CFLAGS = $(FUZZ_CFLAGS)
fuzz: debug
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

purge: clean
	@ cd $(EXT) && $(MAKE) $@
	@ cd $(SRC) && $(MAKE) $@
	@ cd $(TEST) && $(MAKE) $@
	@ rm -rf $(TARGET) $(BIN)
	@ rm -f tags
