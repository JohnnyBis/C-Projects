CC := gcc
LEX := flex
SRCD := src
TSTD := tests
BLDD := build
BIND := bin
INCD := include

MAIN  := $(BLDD)/main.o

ALL_SRCF := $(shell find $(SRCD) -type f -name *.c)
ALL_OBJF := $(patsubst $(SRCD)/%,$(BLDD)/%,$(ALL_SRCF:.c=.o))
ALL_FUNCF := $(filter-out $(MAIN) $(AUX), $(ALL_OBJF))

TEST_ALL_SRCF := $(shell find $(TSTD) -type f -name *.c)
TEST_SRCF := $(filter-out $(TEST_REF_SRCF), $(TEST_ALL_SRCF))

INC := -I $(INCD)

CFLAGS := -Wall -Werror -Wno-unused-variable -Wno-unused-function -MMD
COLORF := -DCOLOR
DFLAGS := -g -DDEBUG -DCOLOR
PRINT_STAMENTS := -DERROR -DSUCCESS -DWARN -DINFO

STD := -std=gnu11
TEST_LIB := -lcriterion
LIBS := -lm
EXTRA := -DLIB_DIR=\"lib/\" \
	-DDEFAULT_INPUT_LANGUAGE=ENGLISH \
	-DDEFAULT_OUTPUT_LANGUAGE=ENGLISH 

CFLAGS += $(STD) $(EXTRA)

EXEC := notation
TEST_EXEC := $(EXEC)_tests


.PHONY: clean all setup debug

all: setup $(BIND)/$(EXEC) $(BIND)/$(TEST_EXEC)

debug: CFLAGS += $(DFLAGS) $(PRINT_STAMENTS) $(COLORF)
debug: all

setup: $(BIND) $(BLDD) lexer.c
$(BIND):
	mkdir -p $(BIND)
$(BLDD):
	mkdir -p $(BLDD)
lexer.c: $(SRCD)/lexer.l $(INCD)/chesstype.h
	$(LEX) -t $(SRCD)/lexer.l > $(SRCD)/lexer.c

$(BIND)/$(EXEC): $(ALL_OBJF)
	$(CC) $^ -o $@ $(LIBS)

$(BIND)/$(TEST_EXEC): $(ALL_FUNCF) $(TEST_SRCF) $(TEST_REF_OBJF)
	$(CC) $(CFLAGS) $(INC) $(ALL_FUNCF) $(TEST_SRCF) $(TEST_REF_OBJF) $(TEST_LIB) $(LIBS) -o $@

$(BLDD)/%.o: $(SRCD)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

$(TSTD)/%.o: $(TSTD)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	strip --strip-unneeded $@

clean:
	rm -rf $(BLDD) $(BIND)

.PRECIOUS: $(BLDD)/*.d
-include $(BLDD)/*.d
