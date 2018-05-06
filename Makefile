default: src

.PHONY: test analysis full-analysis fuzz format tags coverage full-coverage fuzz-coverage fuzz-full-coverage debug purge

export CC = clang

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

# run clang-analyzer
analysis: clean
	@ scan-build $(MAKE)

# run clang-analyzer on dependencies + src
full-analysis: purge
	@ scan-build $(MAKE) -s

src: prepare deps
	@ cd src && $(MAKE) $@

debug: export CFLAGS = -ggdb -Wall -std=c11 -D_GNU_SOURCE -DLOGD_DEBUG 
debug: prepare deps
	@ cd src && $(MAKE) src


test: export CFLAGS =-pthread -Wall -Wno-unused-function -Werror -fsanitize=undefined -fsanitize-coverage=trace-cmp,trace-pc-guard -fprofile-instr-generate -fcoverage-mapping -std=c11 -ggdb -DLOGD_DEBUG -D_GNU_SOURCE
test: clean src
	@ cd $(TEST) && $(MAKE) $@

fuzz: clean src
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
