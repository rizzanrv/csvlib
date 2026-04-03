# csvlib

A lightweight, dependency-free CSV parsing and generation library written in C.

> 🌱 **Note:** I am just starting to learn C, so this library is a learning project. Constructive feedback, bug reports, and pull requests are highly appreciated!

## 📦 Features
- RFC 4180 compliant parsing (handles commas, quotes, escaped quotes, and newlines)
- Simple, flat API with an opaque struct
- Proper memory management & explicit error codes
- Zero external dependencies
- C99 standard compliant
- Cross-platform (POSIX & Windows compatible)

## 🚀 Quick Start
```bash
git clone https://github.com/rizzanrv/csvlib.git
cd csvlib
make
make example && ./examples/basic_usage
```

## 💡 Usage
```c
#include "csv.h"
#include <stdio.h>

int main(void) {
    int err = CSV_OK;
    
    // Read a CSV file
    csv_t *data = csv_read("input.csv", &err);
    if (err != CSV_OK) {
        fprintf(stderr, "Failed to read CSV: %d\n", err);
        return 1;
    }

    // Iterate through rows and columns
    for (size_t r = 0; r < csv_rows(data); r++) {
        for (size_t c = 0; c < csv_cols(data); c++) {
            printf("%s | ", csv_get(data, r, c));
        }
        printf("\n");
    }
    
    // Free allocated memory
    csv_free(data);
    return 0;
}
```

## 📖 API Reference
| Function | Description |
|----------|-------------|
| `csv_create(rows, cols)` | Allocate an empty CSV structure |
| `csv_free(csv)` | Free all allocated memory |
| `csv_read(filename, &error)` | Parse a CSV file from disk |
| `csv_write(csv, filename)` | Write CSV data to disk |
| `csv_get(csv, row, col)` | Get a cell value (returns `NULL` if out of bounds) |
| `csv_set(csv, row, col, value)` | Set a cell value (returns `CSV_OK` or error code) |
| `csv_rows(csv)` / `csv_cols(csv)` | Get dimensions |

## 🛠️ Build & Install
```bash
make          # Compile the static library (libcsv.a)
make example  # Build the usage example
make clean    # Remove build artifacts
sudo make install # Install to /usr/local/include & /usr/local/lib
```

## 🤝 Contributing
1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📜 License
Distributed under the MIT License. See `LICENSE` for details.

## 📬 Contact
**Email:** [me@interes.click](mailto:me@interes.click)  
*Feel free to reach out with questions, suggestions, or collaboration ideas!*