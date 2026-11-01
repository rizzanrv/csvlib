# csvlib

Парсер и генератор CSV на C99 без зависимостей. RFC 4180, поля любого размера,
CRLF, рваные строки, явные коды ошибок.

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

## Сборка

```bash
make                      # libcsv.a
make test                 # 113 проверок, без внешних зависимостей
make example              # пример из examples/basic_usage.c
make test CFLAGS=-fsanitize=address,undefined
sudo make install         # /usr/local/lib + /usr/local/include
```

## API

| Функция | |
| --- | --- |
| `csv_create(rows, cols)` | пустая таблица, ячейки стартуют как `""` |
| `csv_free(csv)` | освободить таблицу |
| `csv_copy(csv)` | глубокая копия |
| `csv_parse(text, len, &err)` | разобрать строку (`len = 0` → `strlen`) |
| `csv_read(path, &err)` | прочитать файл |
| `csv_write(csv, path)` | записать файл (LF) |
| `csv_to_string(csv, &len)` | сериализовать в строку, освобождать через `free()` |
| `csv_get(csv, row, col)` | значение ячейки; `NULL` только при выходе за границы |
| `csv_set(csv, row, col, value)` | записать копию значения (`NULL` → `""`) |
| `csv_push_row(csv, values, count)` | добавить строку, расширив таблицу при необходимости |
| `csv_rows(csv)` / `csv_cols(csv)` | размеры |
| `csv_error_string(code)` | текст ошибки |

Коды: `CSV_OK`, `CSV_ERR_FILE`, `CSV_ERR_MEMORY`, `CSV_ERR_PARSE`,
`CSV_ERR_BOUNDS`, `CSV_ERR_ARG`.

## Поведение

- Каждая строка — ряд, включая пустые (пустая строка = одна ячейка `""`).
- Завершающая запятая даёт пустую ячейку: `a,b,` → `a`, `b`, `""`.
- Файл без перевода строки в конце не добавляет лишний пустой ряд.
- Поля с пробелом в начале или конце при записи берутся в кавычки.
- Незакрытая кавычка не ошибка: остаток ввода становится значением поля.

## Тесты

`tests/test_csv.c` — колода проверок на разбор, кавычки, CRLF, рваные строки,
поля на 20 КБ, границы, копирование, файловый round-trip и запись. CI гоняет
gcc/clang на Linux и macOS плюс отдельную сборку под ASan/UBSan.

## Лицензия

MIT, см. [LICENSE](LICENSE).
