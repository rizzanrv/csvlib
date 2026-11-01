CC       ?= cc
AR       ?= ar
STD      ?= -std=c99
CPPFLAGS ?= -Iinclude
CFLAGS   ?= -Wall -Wextra -Wpedantic -O2
PREFIX   ?= /usr/local

SRC         = src/csv.c
OBJ         = $(SRC:.c=.o)
LIB         = libcsv.a
TEST_BIN    = tests/test_csv
EXAMPLE_BIN = examples/basic_usage

.PHONY: all test example install clean

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $(OBJ)

%.o: %.c include/csv.h
	$(CC) $(STD) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_BIN): tests/test_csv.c $(SRC) include/csv.h
	$(CC) $(STD) $(CPPFLAGS) $(CFLAGS) -g -o $@ tests/test_csv.c $(SRC)

$(EXAMPLE_BIN): examples/basic_usage.c $(LIB) include/csv.h
	$(CC) $(STD) $(CPPFLAGS) $(CFLAGS) -o $@ examples/basic_usage.c -L. -lcsv

test: $(TEST_BIN)
	./$(TEST_BIN)

example: $(EXAMPLE_BIN)
	./$(EXAMPLE_BIN)

install: $(LIB)
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include
	install -m 644 $(LIB) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 include/csv.h $(DESTDIR)$(PREFIX)/include/

clean:
	rm -f $(OBJ) $(LIB) $(TEST_BIN) $(EXAMPLE_BIN)
	rm -f tests/*.csv *.csv
