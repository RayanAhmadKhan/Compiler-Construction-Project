# Compiler Construction Phase 2

This submission contains the Phase 1 table-driven lexer and the Phase 2
table-driven LL(1) predictive parser.

## Project description

This project implements a C++11 table-driven lexer and LL(1) predictive
parser. It tokenizes the source program, generates FIRST and FOLLOW sets and
the parsing table, traces parser-stack actions, reports invalid input, and
uses panic-mode `SCAN`/`POP` recovery. Phase 2 demonstration documentation is
available in [`Phase2Docs/`](Phase2Docs/README.md).

## Requirements

- A C++11 compiler such as `g++`
- No external libraries

## Build and run

### Windows

Run `build_and_run.bat` from this folder. It compiles the program and runs
both required samples.

### Manual command line

```text
g++ -std=c++11 -Wall -Wextra -pedantic main.cpp -o parser
parser SampleA_Valid.txt
parser SampleB_Invalid.txt
```

The executable reads the source path from its first command-line argument.
If no argument is supplied, it reads the included valid `Sourcecode.txt`.

Each run regenerates:

- `Token.txt` and `Error.txt` from the Phase 1 lexer
- `TokenPos.txt` with token locations
- `ParsingTable.txt` with productions, FIRST, FOLLOW, and the generated table
- `ParseTrace.txt` with stack/input/action tracing
- `SyntaxErrors.txt` with panic-mode SCAN/POP recovery
- `ParseResult.txt` with ACCEPT or REJECT status

`SampleA_Valid.txt` must be accepted. `SampleB_Invalid.txt` must be rejected
and its trace must show syntax errors and recovery actions.

Reference outputs from both runs are preserved in `outputs/valid/` and
`outputs/invalid/`.

## Submission checklist

- [x] Updated Phase 1 punctuation set includes comma.
- [x] `Marqazi` is reserved and ordinary identifiers require an underscore.
- [x] `:=` and `!=` are single lexical operators.
- [x] Final grammar is left-recursion-free and left-factored.
- [x] FIRST, FOLLOW, and the complete table are generated in
      `ParsingTable.txt`.
- [x] Table construction reports zero conflicts.
- [x] Parser uses a custom stack and table-driven expansion.
- [x] Panic-mode SCAN/POP actions are logged.
- [x] Valid input is accepted and invalid input is rejected.

## Grammar implementation

The parser uses a left-recursion-free, factored grammar. The production list
and its computed FIRST/FOLLOW sets are emitted to `ParsingTable.txt` at run
time. The parser constructs its table from those productions, detects
conflicts, and uses `$` as the end marker.

Ordinary identifiers must contain an underscore. `Marqazi` is tokenized as a
reserved keyword, and `:=` and `!=` are recognized as single operators.
Comma is included in the Phase 1 punctuation set and is used by both
`ArgList` and `IdentList`.
