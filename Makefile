CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c99 -Iinclude -O2
LDFLAGS =

SRC     = src/csv.c
OBJ     = $(SRC:.c=.o)
LIB     = libcsv.a
EXAMPLE = examples/basic_usage

.PHONY: all clean example

all: $(LIB)

$(LIB): $(OBJ)
	@ar rcs $@ $^
	@echo "Library built: $@"

example: $(EXAMPLE)
	$(CC) $(CFLAGS) -o $@ $< -L. -lcsv
	@echo "Example compiled: $@"

$(EXAMPLE): $(LIB)
	@$(MAKE) $@ -s

clean:
	rm -f $(OBJ) $(LIB) $(EXAMPLE) *.csv

install:
	cp $(LIB) /usr/local/lib/
	cp include/csv.h /usr/local/include/
	@echo "Installed to /usr/local"