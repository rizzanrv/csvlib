/*
 * csvlib example — parse CSV text, mutate it, then write it back to disk.
 * Build & run: make example
 */
#include <stdio.h>
#include <stdlib.h>

#include "csv.h"

static void print_table(const csv_t *csv)
{
    size_t r;
    size_t c;

    printf("%zu row(s) x %zu column(s)\n", csv_rows(csv), csv_cols(csv));
    for (r = 0; r < csv_rows(csv); ++r) {
        for (c = 0; c < csv_cols(csv); ++c) {
            printf("%s%s", csv_get(csv, r, c), (c + 1 == csv_cols(csv)) ? "\n" : " | ");
        }
    }
}

int main(void)
{
    const char *text = "name,age,city\n"
                       "Alice,30,\"New York, NY\"\n"
                       "Bob,25,\"He said \"\"hi\"\"\"\n";
    int err = CSV_OK;
    csv_t *table;
    size_t length = 0;
    char *serialized;

    /* 1. parse straight from a string */
    table = csv_parse(text, 0, &err);
    if (table == NULL) {
        fprintf(stderr, "parse failed: %s\n", csv_error_string(err));
        return 1;
    }
    print_table(table);

    /* 2. mutate it */
    if (csv_set(table, 1, 1, "31") != CSV_OK) {
        fprintf(stderr, "set failed\n");
        csv_free(table);
        return 1;
    }

    /* 3. serialize to memory */
    serialized = csv_to_string(table, &length);
    if (serialized == NULL) {
        fprintf(stderr, "serialization failed\n");
        csv_free(table);
        return 1;
    }
    printf("\n--- %zu bytes ---\n%s", length, serialized);
    free(serialized);

    /* 4. and to disk */
    if (csv_write(table, "example_output.csv") != CSV_OK) {
        fprintf(stderr, "write failed\n");
        csv_free(table);
        return 1;
    }
    csv_free(table);

    /* 5. read it back */
    table = csv_read("example_output.csv", &err);
    if (table == NULL) {
        fprintf(stderr, "read failed: %s\n", csv_error_string(err));
        return 1;
    }
    printf("\n--- re-read ---\n");
    print_table(table);
    csv_free(table);

    remove("example_output.csv");
    return 0;
}
