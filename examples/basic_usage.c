#include <stdio.h>
#include "csv.h"

int main(void) {
    int err = CSV_OK;
    
    // 1. Create & populate
    csv_t *data = csv_create(2, 3);
    if (!data) { fprintf(stderr, "Memory error\n"); return 1; }
    
    csv_set(data, 0, 0, "Name");
    csv_set(data, 0, 1, "Age");
    csv_set(data, 0, 2, "City");
    csv_set(data, 1, 0, "Alice");
    csv_set(data, 1, 1, "30");
    csv_set(data, 1, 2, "New York");

    // 2. Write to file
    csv_write(data, "output.csv");
    csv_free(data);

    // 3. Read back
    csv_t *loaded = csv_read("output.csv", &err);
    if (err != CSV_OK) { fprintf(stderr, "Read failed: %d\n", err); return 1; }

    printf("Rows: %zu, Cols: %zu\n", csv_rows(loaded), csv_cols(loaded));
    for (size_t r = 0; r < csv_rows(loaded); r++) {
        for (size_t c = 0; c < csv_cols(loaded); c++) {
            printf("%s | ", csv_get(loaded, r, c));
        }
        printf("\n");
    }

    csv_free(loaded);
    return 0;
}