default: src

.PHONY: clean test purge analysis full-analysis optimized format tags

export CC = clang++

prepare:
	@ mkdir -p lib bin include/logd

deps:
	@ cd ext && $(MAKE) $@

src: export CFLAGS = -O2
src: prepare deps
	@ cd src && $(MAKE) $@

debug: export CFLAGS = -ggdb
debug: prepare deps
	@ cd src && $(MAKE) $@

test: src
	@ cd test && $(MAKE) $@

clean:
	@ cd src && $(MAKE) $@
	@ cd test && $(MAKE) $@

purge: clean
	@ cd ext && $(MAKE) $@
	@ rm -rf lib bin include

# run clang-analyzer
analysis: clean
	@ scan-build $(MAKE) -s

# run clang-analyzer on dependencies + src
full-analysis: purge
	@ scan-build $(MAKE) -s

format:
	@ find src -name \*.h -o -name \*.c | xargs clang-format -i
	@ find test -name \*.h -o -name \*.c | xargs clang-format -i

tags:
	@ ctags -R
