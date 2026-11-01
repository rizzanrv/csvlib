/*
 * csvlib — RFC 4180 CSV parser/generator.
 * Copyright (c) 2026 rizza. MIT licensed, see LICENSE.
 */
#include "csv.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct csv {
    char **cells; /* rows * cols entries, always non-NULL strings */
    size_t rows;
    size_t cols;
};

/* ---------------- helpers ---------------- */

static char *xstrdup(const char *s)
{
    size_t n;
    char *copy;

    if (s == NULL) {
        s = "";
    }
    n = strlen(s) + 1;
    copy = (char *)malloc(n);
    if (copy != NULL) {
        memcpy(copy, s, n);
    }
    return copy;
}

/* A field needs quoting when it contains a delimiter, a quote, a line break or
   padding whitespace that naive readers would strip. */
static int needs_quote(const char *s)
{
    const char *p;
    size_t len;

    if (s == NULL || s[0] == '\0') {
        return 0;
    }
    for (p = s; *p != '\0'; ++p) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') {
            return 1;
        }
    }
    len = strlen(s);
    if (s[0] == ' ' || s[len - 1] == ' ') {
        return 1;
    }
    return 0;
}

/* ---------------- growable buffers ---------------- */

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} sbuf_t;

static int sbuf_reserve(sbuf_t *b, size_t extra)
{
    size_t need;
    size_t cap;
    char *grown;

    if (extra > (SIZE_MAX - 1) - b->len) {
        return CSV_ERR_MEMORY;
    }
    need = b->len + extra + 1;
    if (need <= b->cap) {
        return CSV_OK;
    }
    cap = (b->cap != 0) ? b->cap : 64;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            return CSV_ERR_MEMORY;
        }
        cap *= 2;
    }
    grown = (char *)realloc(b->data, cap);
    if (grown == NULL) {
        return CSV_ERR_MEMORY;
    }
    b->data = grown;
    b->cap = cap;
    return CSV_OK;
}

static int sbuf_putc(sbuf_t *b, char ch)
{
    int rc = sbuf_reserve(b, 1);
    if (rc != CSV_OK) {
        return rc;
    }
    b->data[b->len] = ch;
    b->len++;
    b->data[b->len] = '\0';
    return CSV_OK;
}

static void sbuf_reset(sbuf_t *b)
{
    b->len = 0;
    if (b->data != NULL) {
        b->data[0] = '\0';
    }
}

static int sbuf_append(sbuf_t *b, const char *text, size_t n)
{
    int rc;

    if (n == 0) {
        return CSV_OK;
    }
    rc = sbuf_reserve(b, n);
    if (rc != CSV_OK) {
        return rc;
    }
    memcpy(b->data + b->len, text, n);
    b->len += n;
    b->data[b->len] = '\0';
    return CSV_OK;
}

static void sbuf_free(sbuf_t *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

/* ---------------- lifecycle ---------------- */

csv_t *csv_create(size_t rows, size_t cols)
{
    csv_t *csv;
    size_t total;
    size_t i;

    if (cols != 0 && rows > SIZE_MAX / cols) {
        return NULL;
    }
    total = rows * cols;

    csv = (csv_t *)calloc(1, sizeof(*csv));
    if (csv == NULL) {
        return NULL;
    }
    csv->rows = rows;
    csv->cols = cols;

    if (total > 0) {
        csv->cells = (char **)calloc(total, sizeof(char *));
        if (csv->cells == NULL) {
            free(csv);
            return NULL;
        }
        for (i = 0; i < total; ++i) {
            csv->cells[i] = xstrdup("");
            if (csv->cells[i] == NULL) {
                csv_free(csv);
                return NULL;
            }
        }
    }
    return csv;
}

void csv_free(csv_t *csv)
{
    size_t total;
    size_t i;

    if (csv == NULL) {
        return;
    }
    total = csv->rows * csv->cols;
    for (i = 0; i < total; ++i) {
        free(csv->cells[i]);
    }
    free(csv->cells);
    free(csv);
}

csv_t *csv_copy(const csv_t *src)
{
    csv_t *dst;
    size_t total;
    size_t i;

    if (src == NULL) {
        return NULL;
    }
    dst = csv_create(src->rows, src->cols);
    if (dst == NULL) {
        return NULL;
    }
    total = src->rows * src->cols;
    for (i = 0; i < total; ++i) {
        char *copy = xstrdup(src->cells[i]);
        if (copy == NULL) {
            csv_free(dst);
            return NULL;
        }
        free(dst->cells[i]);
        dst->cells[i] = copy;
    }
    return dst;
}

/* ---------------- accessors ---------------- */

const char *csv_get(const csv_t *csv, size_t row, size_t col)
{
    const char *value;

    if (csv == NULL || row >= csv->rows || col >= csv->cols) {
        return NULL;
    }
    value = csv->cells[row * csv->cols + col];
    return (value != NULL) ? value : "";
}

int csv_set(csv_t *csv, size_t row, size_t col, const char *value)
{
    size_t index;
    char *copy;

    if (csv == NULL || row >= csv->rows || col >= csv->cols) {
        return CSV_ERR_BOUNDS;
    }
    copy = xstrdup(value);
    if (copy == NULL) {
        return CSV_ERR_MEMORY;
    }
    index = row * csv->cols + col;
    free(csv->cells[index]);
    csv->cells[index] = copy;
    return CSV_OK;
}

size_t csv_rows(const csv_t *csv) { return (csv != NULL) ? csv->rows : 0; }
size_t csv_cols(const csv_t *csv) { return (csv != NULL) ? csv->cols : 0; }

int csv_push_row(csv_t *csv, const char *const *values, size_t count)
{
    size_t new_rows;
    size_t new_cols;
    size_t total;
    char **cells;
    size_t r;
    size_t c;
    size_t base;

    if (csv == NULL || values == NULL || count == 0) {
        return CSV_ERR_ARG;
    }
    new_rows = csv->rows + 1;
    new_cols = (csv->cols > count) ? csv->cols : count;
    if (new_cols != 0 && new_rows > SIZE_MAX / new_cols) {
        return CSV_ERR_MEMORY;
    }
    total = new_rows * new_cols;

    cells = (char **)calloc(total, sizeof(char *));
    if (cells == NULL) {
        return CSV_ERR_MEMORY;
    }
    /* move the existing cells into the (possibly wider) layout */
    for (r = 0; r < csv->rows; ++r) {
        for (c = 0; c < csv->cols; ++c) {
            cells[r * new_cols + c] = csv->cells[r * csv->cols + c];
        }
    }
    base = csv->rows * new_cols;
    for (c = 0; c < new_cols; ++c) {
        const char *source = (c < count) ? values[c] : "";
        cells[base + c] = xstrdup(source);
        if (cells[base + c] == NULL) {
            size_t k;
            for (k = 0; k < c; ++k) {
                free(cells[base + k]);
            }
            free(cells);
            return CSV_ERR_MEMORY;
        }
    }
    free(csv->cells);
    csv->cells = cells;
    csv->rows = new_rows;
    csv->cols = new_cols;
    return CSV_OK;
}

/* ---------------- parser ---------------- */

typedef struct {
    char **fields;
    size_t count;
    size_t cap;
} row_t;

static int row_push(row_t *row, char *owned)
{
    if (row->count == row->cap) {
        size_t cap = (row->cap != 0) ? row->cap * 2 : 8;
        char **grown = (char **)realloc(row->fields, cap * sizeof(char *));
        if (grown == NULL) {
            return CSV_ERR_MEMORY;
        }
        row->fields = grown;
        row->cap = cap;
    }
    row->fields[row->count] = owned;
    row->count++;
    return CSV_OK;
}

static void row_reset(row_t *row)
{
    size_t i;

    for (i = 0; i < row->count; ++i) {
        free(row->fields[i]);
    }
    free(row->fields);
    row->fields = NULL;
    row->count = 0;
    row->cap = 0;
}

static int table_push(row_t **rows, size_t *count, size_t *cap, row_t *row)
{
    if (*count == *cap) {
        size_t cap2 = (*cap != 0) ? *cap * 2 : 16;
        row_t *grown = (row_t *)realloc(*rows, cap2 * sizeof(row_t));
        if (grown == NULL) {
            return CSV_ERR_MEMORY;
        }
        *rows = grown;
        *cap = cap2;
    }
    (*rows)[*count] = *row;
    (*count)++;
    row->fields = NULL;
    row->count = 0;
    row->cap = 0;
    return CSV_OK;
}

/* Push the current field buffer as a cell of `row`. */
static int flush_field(row_t *row, sbuf_t *field)
{
    char *cell = xstrdup(field->data != NULL ? field->data : "");
    int rc;

    if (cell == NULL) {
        return CSV_ERR_MEMORY;
    }
    rc = row_push(row, cell);
    if (rc != CSV_OK) {
        free(cell);
        return rc;
    }
    sbuf_reset(field);
    return CSV_OK;
}

static void discard_all(row_t *rows, size_t count, row_t *current)
{
    size_t i;

    for (i = 0; i < count; ++i) {
        row_reset(&rows[i]);
    }
    free(rows);
    row_reset(current);
}

static csv_t *parse_buffer(const char *text, size_t len, int *error)
{
    sbuf_t field = {NULL, 0, 0};
    row_t current = {NULL, 0, 0};
    row_t *rows = NULL;
    size_t nrows = 0;
    size_t rows_cap = 0;
    size_t max_cols = 0;
    int in_quote = 0;
    int pending = 0;
    int rc = CSV_OK;
    size_t i;
    csv_t *csv = NULL;

    for (i = 0; i < len; ++i) {
        char ch = text[i];

        if (in_quote) {
            if (ch == '"') {
                if (i + 1 < len && text[i + 1] == '"') {
                    rc = sbuf_putc(&field, '"');
                    ++i;
                } else {
                    in_quote = 0;
                }
            } else if (ch == '\r' && i + 1 < len && text[i + 1] == '\n') {
                rc = sbuf_putc(&field, '\n');
                ++i;
            } else {
                rc = sbuf_putc(&field, ch);
            }
            pending = 1;
            if (rc != CSV_OK) {
                break;
            }
            continue;
        }

        if (ch == '"' && field.len == 0) {
            in_quote = 1;
            pending = 1;
        } else if (ch == ',') {
            rc = flush_field(&current, &field);
            pending = 1;
        } else if (ch == '\n' || ch == '\r') {
            rc = flush_field(&current, &field);
            if (rc == CSV_OK) {
                if (ch == '\r' && i + 1 < len && text[i + 1] == '\n') {
                    ++i;
                }
                if (current.count > max_cols) {
                    max_cols = current.count;
                }
                rc = table_push(&rows, &nrows, &rows_cap, &current);
                pending = 0;
            }
        } else {
            rc = sbuf_putc(&field, ch);
            pending = 1;
        }
        if (rc != CSV_OK) {
            break;
        }
    }

    if (rc == CSV_OK && (pending || current.count > 0)) {
        rc = flush_field(&current, &field);
        if (rc == CSV_OK) {
            if (current.count > max_cols) {
                max_cols = current.count;
            }
            rc = table_push(&rows, &nrows, &rows_cap, &current);
        }
    }

    sbuf_free(&field);
    if (rc != CSV_OK) {
        discard_all(rows, nrows, &current);
        if (error != NULL) {
            *error = rc;
        }
        return NULL;
    }

    csv = csv_create(nrows, max_cols);
    if (csv == NULL) {
        discard_all(rows, nrows, &current);
        if (error != NULL) {
            *error = CSV_ERR_MEMORY;
        }
        return NULL;
    }

    {
        size_t r;
        size_t c;

        for (r = 0; r < nrows; ++r) {
            for (c = 0; c < rows[r].count; ++c) {
                char *value = rows[r].fields[c];
                if (value != NULL) {
                    size_t index = r * max_cols + c;
                    free(csv->cells[index]);
                    csv->cells[index] = value;
                    rows[r].fields[c] = NULL;
                }
            }
        }
    }

    discard_all(rows, nrows, &current);
    if (error != NULL) {
        *error = CSV_OK;
    }
    return csv;
}

/* ---------------- public I/O ---------------- */

csv_t *csv_parse(const char *text, size_t length, int *error)
{
    if (text == NULL) {
        if (error != NULL) {
            *error = CSV_ERR_ARG;
        }
        return NULL;
    }
    if (length == 0) {
        length = strlen(text);
    }
    return parse_buffer(text, length, error);
}

csv_t *csv_read(const char *filename, int *error)
{
    FILE *fp;
    sbuf_t buffer = {NULL, 0, 0};
    char chunk[8192];
    size_t got;
    csv_t *csv;
    int read_failed;

    if (filename == NULL) {
        if (error != NULL) {
            *error = CSV_ERR_ARG;
        }
        return NULL;
    }
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        if (error != NULL) {
            *error = CSV_ERR_FILE;
        }
        return NULL;
    }
    while ((got = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        if (sbuf_reserve(&buffer, got) != CSV_OK ||
            buffer.data == NULL) {
            fclose(fp);
            sbuf_free(&buffer);
            if (error != NULL) {
                *error = CSV_ERR_MEMORY;
            }
            return NULL;
        }
        memcpy(buffer.data + buffer.len, chunk, got);
        buffer.len += got;
        buffer.data[buffer.len] = '\0';
    }
    read_failed = ferror(fp);
    fclose(fp);
    if (read_failed) {
        sbuf_free(&buffer);
        if (error != NULL) {
            *error = CSV_ERR_FILE;
        }
        return NULL;
    }

    csv = parse_buffer(buffer.data != NULL ? buffer.data : "", buffer.len, error);
    sbuf_free(&buffer);
    return csv;
}

static int append_field(sbuf_t *out, const char *value)
{
    int rc;

    if (!needs_quote(value)) {
        return sbuf_append(out, value, strlen(value));
    }
    rc = sbuf_putc(out, '"');
    if (rc != CSV_OK) {
        return rc;
    }
    for (; *value != '\0'; ++value) {
        if (*value == '"') {
            rc = sbuf_putc(out, '"');
            if (rc != CSV_OK) {
                return rc;
            }
        }
        rc = sbuf_putc(out, *value);
        if (rc != CSV_OK) {
            return rc;
        }
    }
    return sbuf_putc(out, '"');
}

char *csv_to_string(const csv_t *csv, size_t *out_length)
{
    sbuf_t out = {NULL, 0, 0};
    size_t r;
    size_t c;
    int rc = CSV_OK;

    if (csv == NULL) {
        return NULL;
    }
    if (sbuf_reserve(&out, 1) != CSV_OK) {
        return NULL;
    }
    out.data[0] = '\0';

    for (r = 0; r < csv->rows && rc == CSV_OK; ++r) {
        for (c = 0; c < csv->cols && rc == CSV_OK; ++c) {
            const char *value = csv->cells[r * csv->cols + c];
            rc = append_field(&out, (value != NULL) ? value : "");
            if (rc != CSV_OK) {
                break;
            }
            if (c + 1 == csv->cols) {
                rc = sbuf_putc(&out, '\n');
            } else {
                rc = sbuf_putc(&out, ',');
            }
        }
    }
    if (rc != CSV_OK) {
        sbuf_free(&out);
        return NULL;
    }
    if (out_length != NULL) {
        *out_length = out.len;
    }
    return out.data;
}

int csv_write(const csv_t *csv, const char *filename)
{
    char *text;
    size_t len = 0;
    size_t written;
    FILE *fp;
    int rc;

    if (csv == NULL || filename == NULL) {
        return CSV_ERR_ARG;
    }
    text = csv_to_string(csv, &len);
    if (text == NULL) {
        return CSV_ERR_MEMORY;
    }
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        free(text);
        return CSV_ERR_FILE;
    }
    written = fwrite(text, 1, len, fp);
    rc = (written == len) ? CSV_OK : CSV_ERR_FILE;
    if (fclose(fp) != 0 && rc == CSV_OK) {
        rc = CSV_ERR_FILE;
    }
    free(text);
    return rc;
}

const char *csv_error_string(int error)
{
    switch (error) {
    case CSV_OK:
        return "ok";
    case CSV_ERR_FILE:
        return "file error";
    case CSV_ERR_MEMORY:
        return "out of memory";
    case CSV_ERR_PARSE:
        return "parse error";
    case CSV_ERR_BOUNDS:
        return "index out of bounds";
    case CSV_ERR_ARG:
        return "invalid argument";
    default:
        return "unknown error";
    }
}
