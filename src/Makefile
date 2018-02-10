include ../ext/Makefile

override CFLAGS += -Wall -std=c++11 $(EXT_FEATURES)

INCLUDE_PATH=../include
INCLUDE=$(INCLUDE_PATH)/logd
TARGET =../bin/

EXEC_SRC = logd.cc

EXTOBJ = $(wildcard ../ext/lib/*.o)
SRCS = $(wildcard *.cc)
HDRS = $(addprefix ../include/logd/,$(wildcard *.hpp))
OBJS = $(filter-out $(patsubst %.cc,%.o,$(EXEC_SRC)), $(patsubst %.cc,%.o,$(SRCS)))
EXEC = $(addprefix $(TARGET),$(patsubst %.cc,%,$(EXEC_SRC)))

LIB = ../lib/liblogd.a

.PHONY: clean purge

src: $(LIB) $(EXEC)

%.o: %.cc
	$(CC) $(CFLAGS) -c $<

$(INCLUDE)/%.hpp: %.hpp
	@ cd ../include/logd && \
		ln -s $(subst $(INCLUDE)/,../../src/,$@) $(subst $(INCLUDE)/,,$@)

$(LIB): $(OBJS) $(HDRS)
	@ ar rs $(LIB) $(OBJS) $(EXTOBJ)

$(TARGET)%: %.cc $(LIB)
	@ echo COMPILING $@
	@ $(CC) $(CFLAGS) -I$(INCLUDE_PATH) $< -o $@ $(LIB)

clean:
	@- rm -f $(EXEC)
	@- rm -f $(HDRS)
	@- rm -f $(LIB)
	@- rm -f $(OBJS)

purge: clean
