# mypreprocessor

A small C preprocessor written in C. It reads a C source file, removes comments, expands `#include` directives and substitutes macros, then writes the result to a new file with a `.i` extension.

```
./mypreprocessor inputfile.c     # creates inputfile.i
```

## Features

- **Comment removal**
  - Removes `//` single-line comments and `/* ... */` multi-line comments.
  - Comment markers inside string and character literals are left alone, so `"http://example.com"` is safe.
  - Line structure is preserved, and a block comment is replaced by a single space.
- **File inclusion**
  - Replaces `#include <file>` and `#include "file"` with the contents of the file.
  - Included files are processed too, so nested includes, comments and macros inside them all work.
  - Files are searched for in the folder of the including file, then in the current directory.
- **Macro substitution**
  - Simple macros, for example `#define PI 3.14`.
  - Macros with arguments, for example `#define SQUARE(x) ((x)*(x))`.
  - Nested calls such as `SQUARE(SQUARE(2))` and self-referencing macros are handled without looping forever.
  - Macros are not replaced inside string literals.
  - Multi-line definitions using a trailing `\` are supported.
- **Extras**
  - `#undef`, `#ifdef`, `#ifndef`, `#else` and `#endif`, so include guards in headers work.

## Requirements

- `gcc`
- `make`

On Ubuntu or Debian:

```
sudo apt install build-essential
```

## Build

```
make
```

This compiles every module and creates the executable `mypreprocessor`. To remove the build output:

```
make clean
```

## Usage

```
./mypreprocessor inputfile.c
```

The output is written next to the input file with the extension replaced by `.i`. For example, `./mypreprocessor test.c` creates `test.i`.

## Example

**util.h**

```c
#ifndef UTIL_H
#define UTIL_H
// header comment
#define MAX 100
int add(int a, int b); /* adds */
#endif
```

**test.c**

```c
/* file header
   comment */
#include "util.h"
#define PI 3.14
#define SQUARE(x) ((x)*(x))
#define MAX2(a,b) ((a)>(b)?(a):(b))

int main(void) // entry point
{
    char *s = "PI // stays in string";
    double area = PI * SQUARE(2+3);
    int m = MAX2(SQUARE(2), MAX);
    return 0; /* done */
}
```

Run:

```
./mypreprocessor test.c
cat test.i
```

**test.i**

```c
int add(int a, int b);

int main(void)
{
    char *s = "PI // stays in string";
    double area = 3.14 * ((2+3)*(2+3));
    int m = ((((2)*(2)))>(100)?(((2)*(2))):(100));
    return 0;
}
```

Lines that held only a comment or a `#define` leave blank lines at the top of the file, which keeps the layout of the original.

## Project structure

| File | Purpose |
|---|---|
| `main.c` | Reads the command-line argument, builds the `.i` file name and starts processing |
| `preprocessor.c`, `preprocessor.h` | Controls the line-by-line processing, directives, includes and conditionals |
| `comment_removal.c`, `comment_removal.h` | Removes `//` and `/* */` comments |
| `file_inclusion.c`, `file_inclusion.h` | Reads files, parses `#include` lines and locates the included file |
| `macro_handler.c`, `macro_handler.h` | Stores `#define`s and expands simple and function-like macros |
| `buffer.c`, `buffer.h` | Growable string buffer and safe memory-allocation helpers |
| `Makefile` | Builds the `mypreprocessor` executable |

## How it works

1. `main.c` works out the output file name and opens it.
2. `preprocessor.c` reads the input file and passes the text to `comment_removal.c`.
3. The cleaned text is split into lines. Directive lines that end in `\` are joined with the next line.
4. For each line:
   - `#include` is resolved by `file_inclusion.c`, and the file is processed recursively.
   - `#define` and `#undef` update the macro table in `macro_handler.c`.
   - `#ifdef`, `#ifndef`, `#else` and `#endif` decide whether lines are kept or skipped.
   - Any other line has its macros expanded and is written to the output.

## Behaviour and limitations

- If an included file cannot be found, such as a system header like `<stdio.h>`, a warning is printed and the `#include` line is kept unchanged in the output.
- `#if` and `#elif` lines are passed through unchanged and are not evaluated. `#elif` after `#ifdef` or `#ifndef` is not supported.
- The `#` (stringify) and `##` (token pasting) operators are not supported.
- A macro call whose arguments span several lines is not expanded.
- Macro and parameter names can be at most 63 characters long, and a macro can have at most 10 parameters. A longer name or a bigger parameter list makes the `#define` invalid, and it is ignored with a warning.
- Includes can be nested up to 32 levels deep, which stops a file that includes itself.
- The program exits with status 1 and removes the partial output file if it cannot read the input or hits a fatal error.
