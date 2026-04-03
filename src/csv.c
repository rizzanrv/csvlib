#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Internal structure
struct csv {
    char **cells;
    size_t rows;
    size_t cols;
};

// Helper: safe string duplicate
static char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *d = malloc(len);
    if (d) memcpy(d, s, len);
    return d;
}

// Helper: check if a field needs quoting
static int needs_quote(const char *field) {
    if (!field) return 0;
    for (const char *p = field; *p; p++) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') return 1;
    }
    return 0;
}

// Public: Create
csv_t *csv_create(size_t rows, size_t cols) {
    csv_t *csv = calloc(1, sizeof(csv_t));
    if (!csv) return NULL;
    
    size_t total = rows * cols;
    csv->cells = calloc(total, sizeof(char *));
    if (!csv->cells) { free(csv); return NULL; }
    
    csv->rows = rows;
    csv->cols = cols;
    return csv;
}

// Public: Free
void csv_free(csv_t *csv) {
    if (!csv) return;
    size_t total = csv->rows * csv->cols;
    for (size_t i = 0; i < total; i++) free(csv->cells[i]);
    free(csv->cells);
    free(csv);
}

// Public: Get
const char *csv_get(const csv_t *csv, size_t row, size_t col) {
    if (!csv || row >= csv->rows || col >= csv->cols) return NULL;
    return csv->cells[row * csv->cols + col];
}

// Public: Set
int csv_set(csv_t *csv, size_t row, size_t col, const char *value) {
    if (!csv || row >= csv->rows || col >= csv->cols) return CSV_ERR_BOUNDS;
    
    free(csv->cells[row * csv->cols + col]);
    csv->cells[row * csv->cols + col] = str_dup(value);
    return CSV_OK;
}

// Public: Metadata
size_t csv_rows(const csv_t *csv) { return csv ? csv->rows : 0; }
size_t csv_cols(const csv_t *csv) { return csv ? csv->cols : 0; }

// ---- Parsing Logic ----
typedef struct {
    char **fields;
    size_t count;
    size_t capacity;
} row_buf;

static int row_push(row_buf *rb, const char *val) {
    if (rb->count >= rb->capacity) {
        size_t new_cap = rb->capacity ? rb->capacity * 2 : 16;
        char **tmp = realloc(rb->fields, new_cap * sizeof(char *));
        if (!tmp) return CSV_ERR_MEMORY;
        rb->fields = tmp;
        rb->capacity = new_cap;
    }
    rb->fields[rb->count++] = str_dup(val);
    return CSV_OK;
}

static void row_free(row_buf *rb) {
    for (size_t i = 0; i < rb->count; i++) free(rb->fields[i]);
    free(rb->fields);
}

csv_t *csv_read(const char *filename, int *error) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { if (error) *error = CSV_ERR_FILE; return NULL; }

    row_buf *rows = NULL;
    size_t row_count = 0, row_cap = 0;
    size_t max_cols = 0;
    int err = CSV_OK;

    // Parse state machine
    char field[4096] = {0};
    size_t field_len = 0;
    int in_quote = 0;
    row_buf current_row = {0, 0, 0};

    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '\r') continue; // Skip \r, handle \n as delimiter

        if (in_quote) {
            if (c == '"') {
                int next = fgetc(fp);
                if (next == '"') {
                    field[field_len++] = '"';
                } else {
                    in_quote = 0;
                    ungetc(next, fp);
                }
            } else {
                if (field_len < sizeof(field) - 1) field[field_len++] = (char)c;
            }
        } else {
            if (c == '"') {
                in_quote = 1;
            } else if (c == ',') {
                field[field_len] = '\0';
                err = row_push(&current_row, field);
                if (err) goto cleanup;
                field_len = 0;
            } else if (c == '\n' || c == EOF) {
                field[field_len] = '\0';
                err = row_push(&current_row, field);
                if (err) goto cleanup;
                field_len = 0;

                // Save row
                if (row_count >= row_cap) {
                    size_t nc = row_cap ? row_cap * 2 : 16;
                    row_buf *tmp = realloc(rows, nc * sizeof(row_buf));
                    if (!tmp) { err = CSV_ERR_MEMORY; goto cleanup; }
                    rows = tmp;
                    row_cap = nc;
                }
                rows[row_count++] = current_row;
                current_row = (row_buf){0, 0, 0};
                if (current_row.count > max_cols) max_cols = current_row.count;
            } else {
                if (field_len < sizeof(field) - 1) field[field_len++] = (char)c;
            }
        }
    }

    // Flush last field if file doesn't end with newline
    if (field_len > 0 || in_quote) {
        field[field_len] = '\0';
        err = row_push(&current_row, field);
        if (err == CSV_OK && current_row.count > 0) {
            rows[row_count++] = current_row;
            if (current_row.count > max_cols) max_cols = current_row.count;
            current_row = (row_buf){0, 0, 0};
        }
    }

cleanup:
    fclose(fp);
    if (err != CSV_OK) {
        row_free(&current_row);
        for (size_t i = 0; i < row_count; i++) row_free(&rows[i]);
        free(rows);
        if (error) *error = err;
        return NULL;
    }

    // Build csv_t
    csv_t *csv = csv_create(row_count, max_cols);
    if (!csv) { err = CSV_ERR_MEMORY; goto cleanup2; }

    for (size_t r = 0; r < row_count; r++) {
        for (size_t c = 0; c < rows[r].count; c++) {
            csv->cells[r * max_cols + c] = rows[r].fields[c];
            rows[r].fields[c] = NULL; // Transfer ownership
        }
    }

cleanup2:
    for (size_t i = 0; i < row_count; i++) row_free(&rows[i]);
    free(rows);
    if (err != CSV_OK && error) *error = err;
    return csv;
}

// Public: Write
int csv_write(const csv_t *csv, const char *filename) {
    if (!csv) return CSV_ERR_BOUNDS;
    FILE *fp = fopen(filename, "w");
    if (!fp) return CSV_ERR_FILE;

    size_t total = csv->rows * csv->cols;
    for (size_t i = 0; i < total; i++) {
        const char *val = csv->cells[i] ? csv->cells[i] : "";
        if (needs_quote(val)) {
            fputc('"', fp);
            for (const char *p = val; *p; p++) {
                if (*p == '"') fputc('"', fp);
                fputc(*p, fp);
            }
            fputc('"', fp);
        } else {
            fputs(val, fp);
        }
        if ((i + 1) % csv->cols == 0) fputc('\n', fp);
        else fputc(',', fp);
    }
    fclose(fp);
    return CSV_OK;
}