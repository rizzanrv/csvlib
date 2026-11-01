/*
 * csvlib test suite — no external framework, exits non-zero on failure.
 * Build & run: make test
 */
#include "csv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        g_checks++;                                                            \
        if (!(cond)) {                                                         \
            g_failures++;                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                      \
    } while (0)

#define CHECK_STR(got, want)                                                   \
    do {                                                                       \
        const char *g_ = (got);                                                \
        const char *w_ = (want);                                               \
        g_checks++;                                                            \
        if (g_ == NULL || strcmp(g_, w_) != 0) {                               \
            g_failures++;                                                      \
            fprintf(stderr, "FAIL %s:%d: got \"%s\", want \"%s\"\n",           \
                    __FILE__, __LINE__, g_ != NULL ? g_ : "(null)", w_);       \
        }                                                                      \
    } while (0)

/* ---------------- parsing ---------------- */

static void test_basic_table(void)
{
    const char *text = "name,age,city\nAlice,30,New York\nBob,25,LA\n";
    int err = 1234;
    csv_t *csv = csv_parse(text, 0, &err);

    CHECK(csv != NULL);
    CHECK(err == CSV_OK);
    CHECK(csv_rows(csv) == 3);
    CHECK(csv_cols(csv) == 3);
    CHECK_STR(csv_get(csv, 0, 0), "name");
    CHECK_STR(csv_get(csv, 1, 2), "New York");
    CHECK_STR(csv_get(csv, 2, 1), "25");
    csv_free(csv);

    /* same data without the trailing newline must not add an empty row */
    csv = csv_parse(text, strlen(text) - 1, &err);
    CHECK(csv != NULL);
    CHECK(csv_rows(csv) == 3);
    CHECK_STR(csv_get(csv, 2, 0), "Bob");
    csv_free(csv);
}

static void test_quoting(void)
{
    const char *text = "a,\"b,c\",\"d\"\"e\"\n";
    csv_t *csv = csv_parse(text, 0, NULL);

    CHECK(csv != NULL);
    CHECK(csv_rows(csv) == 1);
    CHECK(csv_cols(csv) == 3);
    CHECK_STR(csv_get(csv, 0, 1), "b,c");
    CHECK_STR(csv_get(csv, 0, 2), "d\"e");
    csv_free(csv);

    /* newline inside quotes belongs to the field, not to the row */
    csv = csv_parse("\"line1\nline2\",x\n", 0, NULL);
    CHECK(csv != NULL);
    CHECK(csv_rows(csv) == 1);
    CHECK(csv_cols(csv) == 2);
    CHECK_STR(csv_get(csv, 0, 0), "line1\nline2");
    CHECK_STR(csv_get(csv, 0, 1), "x");
    csv_free(csv);

    /* CRLF inside quotes is normalised to LF */
    csv = csv_parse("\"a\r\nb\"\r\n", 0, NULL);
    CHECK(csv != NULL);
    CHECK(csv_rows(csv) == 1);
    CHECK_STR(csv_get(csv, 0, 0), "a\nb");
    csv_free(csv);
}

static void test_crlf_and_ragged(void)
{
    csv_t *csv = csv_parse("a,b\r\n1,2\r\n", 0, NULL);

    CHECK(csv != NULL);
    CHECK(csv_rows(csv) == 2);
    CHECK(csv_cols(csv) == 2);
    CHECK_STR(csv_get(csv, 1, 1), "2");
    csv_free(csv);

    /* short rows are padded so csv_get never returns NULL inside bounds */
    csv = csv_parse("a,b,c\n1,2\n", 0, NULL);
    CHECK(csv != NULL);
    CHECK(csv_rows(csv) == 2);
    CHECK(csv_cols(csv) == 3);
    CHECK_STR(csv_get(csv, 1, 2), "");
    csv_free(csv);

    /* trailing comma produces a final empty field */
    csv = csv_parse("a,b,\n", 0, NULL);
    CHECK(csv != NULL);
    CHECK(csv_cols(csv) == 3);
    CHECK_STR(csv_get(csv, 0, 2), "");
    csv_free(csv);
}

static void test_long_field(void)
{
    const size_t size = 20000;
    char *text = (char *)malloc(size + 2);
    csv_t *csv;
    size_t i;

    CHECK(text != NULL);
    if (text == NULL) {
        return;
    }
    memset(text, 'x', size);
    text[size] = '\n';
    text[size + 1] = '\0';

    csv = csv_parse(text, 0, NULL);
    CHECK(csv != NULL);
    CHECK(csv_rows(csv) == 1);
    CHECK(csv_cols(csv) == 1);
    if (csv != NULL) {
        const char *cell = csv_get(csv, 0, 0);
        CHECK(cell != NULL);
        if (cell != NULL) {
            CHECK(strlen(cell) == size);
            for (i = 0; i < size && cell[i] == 'x'; ++i) {
            }
            CHECK(i == size);
        }
    }
    csv_free(csv);
    free(text);
}

static void test_empty_input(void)
{
    int err = 1234;
    csv_t *csv = csv_parse("", 0, &err);

    CHECK(csv != NULL);
    CHECK(err == CSV_OK);
    CHECK(csv_rows(csv) == 0);
    CHECK(csv_cols(csv) == 0);
    CHECK(csv_get(csv, 0, 0) == NULL);
    csv_free(csv);

    /* a single empty line is one row with one empty field */
    csv = csv_parse("\n", 0, NULL);
    CHECK(csv != NULL);
    CHECK(csv_rows(csv) == 1);
    CHECK(csv_cols(csv) == 1);
    CHECK_STR(csv_get(csv, 0, 0), "");
    csv_free(csv);

    csv = csv_parse(NULL, 0, &err);
    CHECK(csv == NULL);
    CHECK(err == CSV_ERR_ARG);
}

/* ---------------- building & round trip ---------------- */

static void test_round_trip(void)
{
    csv_t *csv = csv_create(2, 3);
    csv_t *back;
    char *text;
    size_t len = 0;

    CHECK(csv != NULL);
    if (csv == NULL) {
        return;
    }
    CHECK(csv_set(csv, 0, 0, "plain") == CSV_OK);
    CHECK(csv_set(csv, 0, 1, "with,comma") == CSV_OK);
    CHECK(csv_set(csv, 0, 2, "with \"quote\"") == CSV_OK);
    CHECK(csv_set(csv, 1, 0, "multi\nline") == CSV_OK);
    CHECK(csv_set(csv, 1, 1, " leading space") == CSV_OK);
    CHECK(csv_set(csv, 1, 2, NULL) == CSV_OK);
    CHECK_STR(csv_get(csv, 1, 2), "");

    text = csv_to_string(csv, &len);
    CHECK(text != NULL);
    if (text != NULL) {
        CHECK(len == strlen(text));
        CHECK(strstr(text, "\"with,comma\"") != NULL);
        CHECK(strstr(text, "\"with \"\"quote\"\"\"") != NULL);

        back = csv_parse(text, len, NULL);
        CHECK(back != NULL);
        if (back != NULL) {
            CHECK(csv_rows(back) == 2);
            CHECK(csv_cols(back) == 3);
            CHECK_STR(csv_get(back, 0, 0), "plain");
            CHECK_STR(csv_get(back, 0, 1), "with,comma");
            CHECK_STR(csv_get(back, 0, 2), "with \"quote\"");
            CHECK_STR(csv_get(back, 1, 0), "multi\nline");
            CHECK_STR(csv_get(back, 1, 1), " leading space");
            CHECK_STR(csv_get(back, 1, 2), "");
            csv_free(back);
        }
        free(text);
    }
    csv_free(csv);
}

static void test_push_row(void)
{
    csv_t *csv = csv_create(1, 2);
    const char *row[] = {"a", "b", "c"};

    CHECK(csv != NULL);
    if (csv == NULL) {
        return;
    }
    CHECK(csv_set(csv, 0, 0, "x") == CSV_OK);
    CHECK(csv_set(csv, 0, 1, "y") == CSV_OK);

    CHECK(csv_push_row(csv, row, 3) == CSV_OK);
    CHECK(csv_rows(csv) == 2);
    CHECK(csv_cols(csv) == 3);
    CHECK_STR(csv_get(csv, 0, 0), "x");
    CHECK_STR(csv_get(csv, 0, 1), "y");
    CHECK_STR(csv_get(csv, 0, 2), "");
    CHECK_STR(csv_get(csv, 1, 2), "c");

    CHECK(csv_push_row(csv, row, 0) == CSV_ERR_ARG);
    CHECK(csv_push_row(csv, NULL, 1) == CSV_ERR_ARG);
    csv_free(csv);
}

static void test_bounds_and_copy(void)
{
    csv_t *csv = csv_create(2, 2);
    csv_t *copy;

    CHECK(csv != NULL);
    if (csv == NULL) {
        return;
    }
    CHECK(csv_set(csv, 0, 0, "first") == CSV_OK);
    CHECK(csv_set(csv, 5, 0, "nope") == CSV_ERR_BOUNDS);
    CHECK(csv_set(csv, 0, 5, "nope") == CSV_ERR_BOUNDS);
    CHECK(csv_get(csv, 2, 0) == NULL);
    CHECK(csv_get(csv, 0, 2) == NULL);
    CHECK(csv_get(NULL, 0, 0) == NULL);
    CHECK(csv_rows(NULL) == 0);

    copy = csv_copy(csv);
    CHECK(copy != NULL);
    if (copy != NULL) {
        CHECK_STR(csv_get(copy, 0, 0), "first");
        CHECK(csv_set(copy, 0, 0, "changed") == CSV_OK);
        CHECK_STR(csv_get(copy, 0, 0), "changed");
        CHECK_STR(csv_get(csv, 0, 0), "first");
        csv_free(copy);
    }
    csv_free(csv);
    csv_free(NULL);
}

static void test_file_io(void)
{
    const char *path = "tests/tmp_roundtrip.csv";
    csv_t *csv = csv_create(2, 2);
    csv_t *back;
    int err = 1234;

    CHECK(csv != NULL);
    if (csv == NULL) {
        return;
    }
    CHECK(csv_set(csv, 0, 0, "key") == CSV_OK);
    CHECK(csv_set(csv, 0, 1, "value, with comma") == CSV_OK);
    CHECK(csv_set(csv, 1, 0, "k2") == CSV_OK);
    CHECK(csv_set(csv, 1, 1, "v2") == CSV_OK);

    CHECK(csv_write(csv, path) == CSV_OK);
    csv_free(csv);

    back = csv_read(path, &err);
    CHECK(back != NULL);
    CHECK(err == CSV_OK);
    if (back != NULL) {
        CHECK(csv_rows(back) == 2);
        CHECK_STR(csv_get(back, 0, 1), "value, with comma");
        CHECK_STR(csv_get(back, 1, 1), "v2");
        csv_free(back);
    }
    remove(path);

    back = csv_read("tests/definitely_missing.csv", &err);
    CHECK(back == NULL);
    CHECK(err == CSV_ERR_FILE);
    CHECK(csv_write(NULL, path) == CSV_ERR_ARG);
    CHECK(csv_error_string(CSV_ERR_FILE) != NULL);
}

int main(void)
{
    test_basic_table();
    test_quoting();
    test_crlf_and_ragged();
    test_long_field();
    test_empty_input();
    test_round_trip();
    test_push_row();
    test_bounds_and_copy();
    test_file_io();

    printf("%d checks, %d failure(s)\n", g_checks, g_failures);
    return (g_failures == 0) ? 0 : 1;
}
