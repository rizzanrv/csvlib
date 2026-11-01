#ifndef CSV_H
#define CSV_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle to a CSV table. */
typedef struct csv csv_t;

/* Error codes: 0 = success, negative = failure. */
#define CSV_OK          0
#define CSV_ERR_FILE   -1
#define CSV_ERR_MEMORY -2
#define CSV_ERR_PARSE  -3
#define CSV_ERR_BOUNDS -4
#define CSV_ERR_ARG    -5

/* ---- lifecycle ---- */

/* Allocate an empty rows x cols table. Cells default to "" (never NULL).
   Returns NULL on allocation failure or on rows*cols overflow. */
csv_t *csv_create(size_t rows, size_t cols);

/* Free the table and every cell. Safe with NULL. */
void csv_free(csv_t *csv);

/* Deep copy. Returns NULL on allocation failure. */
csv_t *csv_copy(const csv_t *csv);

/* ---- I/O ---- */

/* Parse a file. On failure returns NULL and, if error != NULL, stores a
   CSV_ERR_* code. Handles RFC 4180 quoting, CRLF, ragged rows and fields of
   any length. */
csv_t *csv_read(const char *filename, int *error);

/* Parse in-memory text; pass length = 0 to use strlen(text). */
csv_t *csv_parse(const char *text, size_t length, int *error);

/* Serialize to a file (RFC 4180, LF line endings). */
int csv_write(const csv_t *csv, const char *filename);

/* Serialize to a heap string. out_length (optional) receives the byte count
   excluding the terminator. The caller must free() the result. */
char *csv_to_string(const csv_t *csv, size_t *out_length);

/* ---- accessors ---- */

/* Cell value. Returns "" for a padded cell, NULL only when the index is out
   of bounds or csv is NULL. */
const char *csv_get(const csv_t *csv, size_t row, size_t col);

/* Store a copy of value (NULL stores ""). */
int csv_set(csv_t *csv, size_t row, size_t col, const char *value);

/* ---- mutation ---- */

/* Append a row of count values, widening the table if the row is longer than
   the current column count. Existing rows are padded with "". */
int csv_push_row(csv_t *csv, const char *const *values, size_t count);

/* ---- metadata ---- */

size_t csv_rows(const csv_t *csv);
size_t csv_cols(const csv_t *csv);

/* Text for a CSV_ERR_* code. */
const char *csv_error_string(int error);

#ifdef __cplusplus
}
#endif

#endif /* CSV_H */
