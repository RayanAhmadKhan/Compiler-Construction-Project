# Phase 2 Documentation

This folder contains documentation for the table-driven LL(1) predictive
parser developed for Compiler Construction Phase 2.

## Demonstrated behavior

- Successful parsing of `SampleA_Valid.txt`
- Invalid-input detection using `SampleB_Invalid.txt`
- Panic-mode error recovery with `POP`
- `SCAN` recovery when unexpected input remains after the parser reaches `$`
- Generated tokens, parser traces, syntax errors, and final parse results

## Demo video outline

1. Run the parser with the valid sample and show the `ACCEPT` result.
2. Run the parser with the invalid sample and show the syntax errors.
3. Open `ParseTrace.txt` and `SyntaxErrors.txt` to show the two `POP`
   recovery actions.
4. Demonstrate `SCAN` with an additional unexpected token after the closing
   brace and show the corresponding trace action.
5. Summarize the difference between `ACCEPT`, `REJECT`, `POP`, and `SCAN`.

## Generated evidence

The parser writes the following files in the project root:

- `Token.txt` and `TokenPos.txt` - lexer tokens and their locations
- `ParsingTable.txt` - productions, FIRST/FOLLOW sets, and LL(1) table
- `ParseTrace.txt` - parser stack, input, productions, and actions
- `SyntaxErrors.txt` - syntax diagnostics and recovery actions
- `ParseResult.txt` - final `ACCEPT` or `REJECT` result

Reference outputs for the valid and invalid samples are preserved in
`outputs/valid/` and `outputs/invalid/`.
