#ifndef CSV_H
#define CSV_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque struct for CSV data
typedef struct csv csv_t;

// Error codes
#define CSV_OK          0
#define CSV_ERR_FILE   -1
#define CSV_ERR_MEMORY -2
#define CSV_ERR_PARSE  -3
#define CSV_ERR_BOUNDS -4

// Lifecycle
csv_t *csv_create(size_t rows, size_t cols);
void   csv_free(csv_t *csv);

// I/O
csv_t *csv_read(const char *filename, int *error);
int    csv_write(const csv_t *csv, const char *filename);

// Accessors
const char *csv_get(const csv_t *csv, size_t row, size_t col);
int         csv_set(csv_t *csv, size_t row, size_t col, const char *value);

// Metadata
size_t csv_rows(const csv_t *csv);
size_t csv_cols(const csv_t *csv);

#ifdef __cplusplus
}
#endif

#endif // CSV_H