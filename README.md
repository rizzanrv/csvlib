# csvlib

[![ci](https://github.com/rizzanrv/csvlib/actions/workflows/ci.yml/badge.svg)](https://github.com/rizzanrv/csvlib/actions/workflows/ci.yml)
[![license: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![c99](https://img.shields.io/badge/C-C99-informational)

A small, dependency-free CSV parser and writer in C99. RFC 4180 quoting,
CRLF handling and fields of any length — no fixed buffers, no globals, no
`malloc` surprises.

```c
int err = CSV_OK;
csv_t *csv = csv_parse("name,age\nAlice,30\n", 0, &err);
if (csv == NULL) {
    fprintf(stderr, "%s\n", csv_error_string(err));
    return 1;
}
for (size_t r = 0; r < csv_rows(csv); ++r) {
    for (size_t c = 0; c < csv_cols(csv); ++c) {
        printf("%s%s", csv_get(csv, r, c), (c + 1 == csv_cols(csv)) ? "\n" : " | ");
    }
}
csv_free(csv);
```

## Features

- **RFC 4180**: quoted fields, `""` escapes, embedded commas and newlines.
- **CRLF and LF** input; `\r\n` inside quotes is normalised to `\n`.
- **Ragged rows** are padded with `""`, so `csv_get` never hands back `NULL`
  for a valid index.
- **Unlimited field size**: the parser grows dynamically instead of using a
  fixed buffer.
- **Explicit ownership**: `csv_get` returns a pointer owned by the table;
  `csv_set` copies the string you pass in.
- **Explicit error codes** (`CSV_ERR_*`) plus `csv_error_string()`.
- No dependencies, no I/O beyond `fopen`/`fread`, C99 and C++ friendly
  (`extern "C"` guards).

## Quick start

```bash
git clone https://github.com/rizzanrv/csvlib.git
cd csvlib
make            # builds libcsv.a
make test       # builds and runs the test suite
make example    # builds and runs examples/basic_usage
```

Drop `include/csv.h` and `libcsv.a` into your project, or:

```bash
sudo make install     # /usr/local/lib + /usr/local/include
```

## API

| Function | Description |
| --- | --- |
| `csv_create(rows, cols)` | Allocate an empty table; every cell starts as `""` |
| `csv_free(csv)` | Free the table and all cells (NULL-safe) |
| `csv_copy(csv)` | Deep copy, returns `NULL` on OOM |
| `csv_parse(text, len, &err)` | Parse in-memory text (`len = 0` → `strlen`) |
| `csv_read(path, &err)` | Read and parse a file |
| `csv_write(csv, path)` | Serialize to a file (LF line endings) |
| `csv_to_string(csv, &len)` | Serialize to a heap string; caller `free()`s it |
| `csv_get(csv, row, col)` | Cell value; `NULL` only when out of bounds |
| `csv_set(csv, row, col, value)` | Store a copy of `value` (`NULL` stores `""`) |
| `csv_push_row(csv, values, count)` | Append a row, widening the table if needed |
| `csv_rows(csv)` / `csv_cols(csv)` | Dimensions |
| `csv_error_string(code)` | Human readable error text |

Error codes: `CSV_OK` (0), `CSV_ERR_FILE`, `CSV_ERR_MEMORY`, `CSV_ERR_PARSE`,
`CSV_ERR_BOUNDS`, `CSV_ERR_ARG`.

## Behaviour notes

- Every line is a row — including blank lines (a blank line is one row with a
  single empty field).
- A trailing comma creates a trailing empty field (`a,b,` → `a`, `b`, `""`).
- A file that ends without a newline does **not** gain an extra empty row.
- Fields that begin or end with a space are quoted on output so naive readers
  do not strip them.
- An unterminated quote is accepted leniently: the rest of the input becomes
  the field value instead of failing the parse.

## Testing

```bash
make test                                    # 113 assertions
make test CFLAGS=-fsanitize=address,undefined   # memory/UB checks
```

CI runs the suite with GCC and Clang on Linux and macOS, plus an
AddressSanitizer/UBSan job.

## Project layout

```
include/csv.h      public API
src/csv.c          parser, writer, table management
tests/test_csv.c   dependency-free test suite
examples/          usage example
```

## License

MIT — see [LICENSE](LICENSE).

## Feedback

Bug reports and pull requests are welcome. This started as a learning project;
the parser was later rewritten for correctness (dynamic field buffer, no
uninitialised column count, no writes past the row array), so regressions in
edge cases are exactly what the test suite is for.
