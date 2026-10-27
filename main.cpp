// ============================================================================
//  Compiler Construction - Project Phase 1 (Lexer) + Phase 2 (LL(1) Parser)
//
//  Flow:  Sourcecode.txt --> table-driven lexer --> Token.txt / Error.txt
//         Token.txt --> stack-based LL(1) predictive parser
//                   --> ParseTrace.txt / SyntaxErrors.txt / ParseResult.txt  (+ TokenPos.txt: line/col of each token)
//         (ParsingTable.txt = FIRST/FOLLOW/table dump, useful for the handwritten part)
//
//  No std::string / std::vector / std::stack are used: stack, token list and
//  parsing-table storage are all hand-written.
//
//  Usage:  ./main [source_file]      (default source file: Sourcecode.txt)
// ============================================================================
#include <iostream>
#include <fstream>

using namespace std;

// ----------------------------------------------------------------------------
//  Small string helpers (no <cstring>)
// ----------------------------------------------------------------------------
int my_strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

bool my_strcmp(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) return false;
        i++;
    }
    return str1[i] == str2[i];
}

char* my_strdup(const char* s) {
    int n = my_strlen(s);
    char* r = new char[n + 1];
    for (int i = 0; i <= n; i++) r[i] = s[i];
    return r;
}

// ============================================================================
//  PHASE 1 : TABLE-DRIVEN LEXER
// ============================================================================

struct LexemeBuffer {
    char* data;
    int length;
    int capacity;

    LexemeBuffer() {
        capacity = 16;
        length = 0;
        data = new char[capacity];
        data[0] = '\0';
    }

    ~LexemeBuffer() {
        delete[] data;
    }

    void append(char c) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char* newData = new char[capacity];
            for (int i = 0; i < length; i++) newData[i] = data[i];
            delete[] data;
            data = newData;
        }
        data[length++] = c;
        data[length] = '\0';
    }

    void clear() {
        length = 0;
        data[0] = '\0';
    }
};

const char* keywords[] = {
    "asm", "Wagarna", "new", "this", "auto", "enum", "operator", "throw", "Mantiqi",
    "explicit", "private", "True", "break", "export", "protected", "try", "case",
    "extern", "public", "typedef", "catch", "False", "register", "typeid", "Harf",
    "Ashriya", "typename", "Adadi", "class", "for", "Wapas", "union", "const",
    "dost", "short", "unsigned", "goto", "signed", "using", "continue", "Agar",
    "sizeof", "virtual", "default", "inline", "static", "Khali", "delete", "volatile",
    "do", "long", "struct", "double", "mutable", "switch", "while", "namespace",
    "template", "Marqazi", "Matn", "input->", "output<-"
};
const int NUM_KEYWORDS = sizeof(keywords) / sizeof(keywords[0]);

bool is_keyword(const char* lexeme) {
    for (int i = 0; i < NUM_KEYWORDS; i++) {
        if (my_strcmp(lexeme, keywords[i])) return true;
    }
    return false;
}

const char* operators[] = {
    "<", ">", "<>", ":=", "==", "+", "-", "++", "+=", "<=", ">=",
    "%", "||", "&&", "!=", "*", "/", "<<", ">>"
};

const int NUM_OPERATORS = sizeof(operators) / sizeof(operators[0]);

bool is_operator(const char* lexeme) {
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (my_strcmp(lexeme, operators[i])) return true;
    }
    return false;
}

enum CharClass {
    CC_LETTER = 0, CC_E = 1, CC_DIGIT = 2, CC_UNDERSCORE = 3, CC_DOT = 4,
    CC_PLUS_MINUS = 5, CC_QUOTE = 6, CC_OPERATOR = 7, CC_COLON = 8, CC_PUNC = 9,
    CC_SPACE = 10, CC_OTHER = 11, CC_COUNT = 12
};

enum State {
    S_START = 0, S_ID_NO_US = 1, S_ID_VALID = 2, S_NUM_INT = 3, S_SIGN_START = 4,
    S_NUM_FLOAT_WAIT = 5, S_NUM_FLOAT = 6, S_NUM_EXP_WAIT = 7, S_NUM_EXP_SIGN = 8,
    S_NUM_EXP = 9, S_STRING = 10, S_STRING_DONE = 11, S_OP = 12, S_COLON = 13,
    S_COLON_DONE = 14, S_PUNC = 15, S_ERROR = 16, S_DONE = -1
};

CharClass get_char_class(char c) {
    if (c == 'E' || c == 'e') return CC_E;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return CC_LETTER;
    if (c >= '0' && c <= '9') return CC_DIGIT;
    if (c == '_') return CC_UNDERSCORE;
    if (c == '.') return CC_DOT;
    if (c == '+' || c == '-') return CC_PLUS_MINUS;
    if (c == '"') return CC_QUOTE;
    if (c == '<' || c == '>' || c == '=' || c == '!' || c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == ';') return CC_OPERATOR;
    if (c == ':') return CC_COLON;
    // PHASE 2 UPDATE: ',' (comma) added to the punctuation set
    if (c == '[' || c == ']' || c == '{' || c == '}' || c == '(' || c == ')' || c == ',') return CC_PUNC;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return CC_SPACE;
    return CC_OTHER;
}

int dfa[17][CC_COUNT] = {
    // LTR,         E,            DIGIT,      UNDERSCORE, DOT,              PLUS_MINUS,   QUOTE,         OPERATOR, COLON,        PUNC,    SPACE,   OTHER
    {S_ID_NO_US,   S_ID_NO_US,   S_NUM_INT,  S_ID_VALID, S_ERROR,          S_SIGN_START, S_STRING,      S_OP,     S_COLON,      S_PUNC,  S_START, S_ERROR}, // 0: START
    {S_ID_NO_US,   S_ID_NO_US,   S_ID_NO_US, S_ID_VALID, S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 1: ID_NO_US
    {S_ID_VALID,   S_ID_VALID,   S_ID_VALID, S_ID_VALID, S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 2: ID_VALID
    {S_DONE,       S_NUM_EXP_WAIT,S_NUM_INT, S_DONE,     S_NUM_FLOAT_WAIT, S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 3: NUM_INT
    {S_DONE,       S_DONE,       S_NUM_INT,  S_DONE,     S_NUM_FLOAT_WAIT, S_OP,         S_DONE,        S_OP,     S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 4: SIGN_START
    {S_ERROR,      S_ERROR,      S_NUM_FLOAT,S_ERROR,    S_ERROR,          S_ERROR,      S_ERROR,       S_ERROR,  S_ERROR,      S_ERROR, S_ERROR, S_ERROR}, // 5: NUM_FLOAT_WAIT
    {S_DONE,       S_NUM_EXP_WAIT,S_NUM_FLOAT,S_DONE,    S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 6: NUM_FLOAT
    {S_ERROR,      S_ERROR,      S_NUM_EXP,  S_ERROR,    S_ERROR,          S_NUM_EXP_SIGN,S_ERROR,      S_ERROR,  S_ERROR,      S_ERROR, S_ERROR, S_ERROR}, // 7: NUM_EXP_WAIT
    {S_ERROR,      S_ERROR,      S_NUM_EXP,  S_ERROR,    S_ERROR,          S_ERROR,      S_ERROR,       S_ERROR,  S_ERROR,      S_ERROR, S_ERROR, S_ERROR}, // 8: NUM_EXP_SIGN
    {S_DONE,       S_DONE,       S_NUM_EXP,  S_DONE,     S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 9: NUM_EXP
    {S_STRING,     S_STRING,     S_STRING,   S_STRING,   S_STRING,         S_STRING,     S_STRING_DONE, S_STRING, S_STRING,     S_STRING,S_STRING,S_STRING},// 10: STRING
    {S_DONE,       S_DONE,       S_DONE,     S_DONE,     S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 11: STRING_DONE
    {S_DONE,       S_DONE,       S_DONE,     S_DONE,     S_DONE,           S_OP,         S_DONE,        S_OP,     S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 12: OP
    {S_DONE,       S_DONE,       S_DONE,     S_DONE,     S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_COLON_DONE, S_DONE,  S_DONE,  S_DONE},  // 13: COLON
    {S_DONE,       S_DONE,       S_DONE,     S_DONE,     S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 14: COLON_DONE
    {S_DONE,       S_DONE,       S_DONE,     S_DONE,     S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE},  // 15: PUNC
    {S_DONE,       S_DONE,       S_DONE,     S_DONE,     S_DONE,           S_DONE,       S_DONE,        S_DONE,   S_DONE,       S_DONE,  S_DONE,  S_DONE}   // 16: ERROR
};

// Counts every lexical error written to Error.txt.
#define LEXERR (++lexErrors, errFile)

// ---- line / column tracking (needed for "line L col C" in syntax errors) ----
// Token.txt keeps the Phase 1 format "< KIND, lexeme >". The start position of
// every token is written, in the same order, to TokenPos.txt as "line col".
int gLine = 1, gCol = 0;
int histLine[16], histCol[16], histN = 0;

bool readCh(ifstream& in, char& c) {
    if (!in.get(c)) return false;
    if (histN == 16) {
        for (int i = 1; i < 16; i++) { histLine[i - 1] = histLine[i]; histCol[i - 1] = histCol[i]; }
        histN = 15;
    }
    histLine[histN] = gLine; histCol[histN] = gCol; histN++;
    if (c == '\n') { gLine++; gCol = 0; } else gCol++;
    return true;
}

void unreadCh(ifstream& in, char c) {
    in.putback(c);
    if (histN > 0) { histN--; gLine = histLine[histN]; gCol = histCol[histN]; }
}

void emitTok(ofstream& tok, ofstream& pos, const char* kind, const char* lex, int line, int col) {
    tok << "< " << kind << ", " << lex << " >\n";
    pos << line << " " << col << "\n";
}
#define EMIT(K) emitTok(tokFile, posFile, K, lexeme.data, tokLine, tokCol)

// Returns 0 on success, 1 if the source file cannot be opened.
int runLexer(const char* sourcePath, int& lexErrors) {
    ifstream inFile(sourcePath);
    ofstream tokFile("Token.txt");
    ofstream errFile("Error.txt");
    ofstream posFile("TokenPos.txt");
    lexErrors = 0;
    gLine = 1; gCol = 0; histN = 0;
    int tokLine = 1, tokCol = 1;

    if (!inFile.is_open()) {
        cout << "Critical Error: Could not open " << sourcePath << endl;
        return 1;
    }

    LexemeBuffer lexeme;
    int currentState = S_START;
    char c;

    while (readCh(inFile, c)) {
        CharClass cc = get_char_class(c);
        int nextState = dfa[currentState][cc];

        if (nextState == S_DONE) {
            bool consumedCurrentCharacter = false;
            // Process the completed token based on the LAST valid state
            if (currentState == S_ID_NO_US) {
                // The arrow keywords are recognized with the delimiter already read.
                if ((my_strcmp(lexeme.data, "input") && c == '-') ||
                    (my_strcmp(lexeme.data, "output") && c == '<')) {
                    char second = '\0';
                    bool got = readCh(inFile, second);
                    if (got && ((c == '-' && second == '>') || (c == '<' && second == '-'))) {
                        lexeme.append(c);
                        lexeme.append(second);
                        EMIT("KEYWORD");
                        consumedCurrentCharacter = true;
                    } else {
                        inFile.clear();
                        if (got) unreadCh(inFile, second);
                        LEXERR << "Lexical Error (Invalid arrow keyword): " << lexeme.data << c << "\n";
                        // c itself is put back once, at the bottom of this block
                    }
                } else if (is_keyword(lexeme.data)) {
                    EMIT("KEYWORD");
                } else {
                    LEXERR << "Lexical Error (Invalid ID, missing underscore): " << lexeme.data << "\n";
                }
            }
            else if (currentState == S_ID_VALID) { EMIT("IDENTIFIER"); }
            else if (currentState == S_NUM_INT || currentState == S_SIGN_START) {
                // SIGN_START means it was just a '+' or '-' without digits
                if (currentState == S_SIGN_START) EMIT("OPERATOR");
                else EMIT("INTEGER");
            }
            else if (currentState == S_NUM_FLOAT) { EMIT("FLOAT"); }
            else if (currentState == S_NUM_EXP) { EMIT("EXPONENTIAL"); }
            else if (currentState == S_STRING_DONE) { EMIT("QUOTED_LITERAL"); }
            else if (currentState == S_OP) {
                if (is_operator(lexeme.data)) EMIT("OPERATOR");
                else LEXERR << "Lexical Error (Invalid operator): " << lexeme.data << "\n";
            }
            else if (currentState == S_COLON && c == '=') {
                lexeme.append(c);
                EMIT("OPERATOR");
                consumedCurrentCharacter = true;
            }
            else if (currentState == S_COLON_DONE || currentState == S_PUNC) {
                EMIT("PUNCTUATION");
            }
            else if (currentState == S_COLON) {
                LEXERR << "Lexical Error (Expected ::): " << lexeme.data << "\n";
            }
            else if (currentState == S_ERROR) {
                LEXERR << "Lexical Error (Malformed token): " << lexeme.data << "\n";
            }

            // Reset and reprocess the current character for the next token
            lexeme.clear();
            currentState = S_START;
            if (!consumedCurrentCharacter) unreadCh(inFile, c);
        }
        else {
            // Valid transition: append character and move state
            if (nextState != S_START || cc != CC_SPACE) { // Don't append leading whitespace
                if (lexeme.length == 0) { tokLine = gLine; tokCol = gCol; }
                lexeme.append(c);
            }
            currentState = nextState;
        }
    }

    // Handle EOF if a token was left in the buffer
    if (lexeme.length > 0) {
        if (currentState == S_ID_VALID) EMIT("IDENTIFIER");
        else if (currentState == S_ID_NO_US && is_keyword(lexeme.data)) EMIT("KEYWORD");
        else if (currentState == S_NUM_INT) EMIT("INTEGER");
        else if (currentState == S_NUM_FLOAT) EMIT("FLOAT");
        else if (currentState == S_NUM_EXP) EMIT("EXPONENTIAL");
        else if (currentState == S_STRING_DONE) EMIT("QUOTED_LITERAL");
        else if (currentState == S_OP && is_operator(lexeme.data)) EMIT("OPERATOR");
        else if (currentState == S_SIGN_START && is_operator(lexeme.data)) EMIT("OPERATOR");
        else if (currentState == S_PUNC || currentState == S_COLON_DONE) EMIT("PUNCTUATION");
        else if (currentState == S_COLON) LEXERR << "Lexical Error (Expected ::): " << lexeme.data << "\n";
        else if (currentState == S_OP) LEXERR << "Lexical Error (Invalid operator): " << lexeme.data << "\n";
        else LEXERR << "Lexical Error (Unexpected EOF): " << lexeme.data << "\n";
    }

    inFile.close();
    tokFile.close();
    errFile.close();
    posFile.close();

    cout << "Scanning complete. Check Token.txt and Error.txt" << endl;
    return 0;
}

// ============================================================================
//  PHASE 2 : LL(1) PREDICTIVE PARSER   (matches the handwritten solution)
// ============================================================================
//
//  FINAL LL(1) GRAMMAR (Part B).  Production numbers are used in the parsing
//  table (Part E) and in the traces (Part F).  eps = epsilon.
//
//    1 Program   -> Adadi ProgTail          23 Stmt     -> WhileStmt
//    2 Program   -> Ashriya FuncRest        24 Stmt     -> Expr ::
//    3 Program   -> Harf FuncRest           25 Stmt     -> IfStmt
//    4 Program   -> Matn FuncRest           26 Stmt     -> CompStmt
//    5 Program   -> Mantiqi FuncRest        27 Stmt     -> Declaration
//    6 ProgTail  -> Marqazi ( ) CompStmt    28 Stmt     -> ::
//    7 ProgTail  -> FuncRest                29 ForStmt  -> for ( Expr :: OptExpr :: OptExpr ) Stmt
//    8 FuncRest  -> ID ( ArgList ) CompStmt 30 OptExpr  -> Expr
//    9 ArgList   -> Arg ArgList'            31 OptExpr  -> eps
//   10 ArgList'  -> , Arg ArgList'          32 WhileStmt-> while ( Expr ) Stmt
//   11 ArgList'  -> eps                     33 IfStmt   -> Agar ( Expr ) CompStmt ElsePart
//   12 Arg       -> Type ID                 34 ElsePart -> Wagarna CompStmt
//   13 Declaration -> Type IdentList ::     35 ElsePart -> eps
//   14 IdentList -> ID IdentList'           36 CompStmt -> { StmtList }
//   15 IdentList'-> , IdentList             37 StmtList -> Stmt StmtList
//   16 IdentList'-> eps                     38 StmtList -> eps
//   17-21 Type -> Adadi|Ashriya|Harf|Matn|Mantiqi
//   22 Stmt      -> ForStmt
//   39 Expr     -> ID ExprRest              54 Mag      -> Term Mag'
//   40 Expr     -> NUM RvTail               55 Mag'     -> + Term Mag'
//   41 Expr     -> ( Expr ) RvTail          56 Mag'     -> - Term Mag'
//   42 ExprRest -> := Expr                  57 Mag'     -> eps
//   43 ExprRest -> RvTail                   58 Term     -> Factor Term'
//   44 RvTail   -> Term' Mag' Rvalue'       59 Term'    -> * Factor Term'
//   45 Rvalue'  -> Compare Mag Rvalue'      60 Term'    -> / Factor Term'
//   46 Rvalue'  -> eps                      61 Term'    -> eps
//   47-53 Compare -> == | < | > | <= | >= | != | <>
//   62 Factor   -> ( Expr )    63 Factor -> ID    64 Factor -> NUM
//
//  Terminals: Adadi Ashriya Harf Matn Mantiqi Marqazi for while Agar Wagarna
//             ID NUM ( ) { } , :: := == < > <= >= != <> + - * /  $
//  (any other token of Phase 1 is mapped to "other" and is always SCANned)
//
//  Table M[A,a]: production number if a in FIRST(alpha) (or FOLLOW(A) when
//  alpha is nullable); otherwise POP when a in FOLLOW(A) or a = $, else SCAN.
//  The table is generated from the production list below (FIRST/FOLLOW are
//  computed in code) and is written to ParsingTable.txt in the same layout
//  as Part E of the handwritten solution.
// ----------------------------------------------------------------------------

// Grammar symbols: terminals first (0..T_COUNT-1), then nonterminals.
enum Sym {
    T_ADADI = 0, T_ASHRIYA, T_HARF, T_MATN, T_MANTIQI, T_MARQAZI,
    T_FOR, T_WHILE, T_AGAR, T_WAGARNA,
    T_ID, T_NUM,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_COMMA, T_DCOLON, T_ASSIGN,
    T_EQ, T_LT, T_GT, T_LE, T_GE, T_NE, T_DIAMOND,
    T_PLUS, T_MINUS, T_STAR, T_SLASH,
    T_END,       // '$' end marker
    T_UNKNOWN,   // "other": any token that is not part of the Phase 2 grammar
    T_COUNT,

    N_PROGRAM = T_COUNT, N_PROGTAIL, N_FUNCREST, N_ARGLIST, N_ARGLISTP, N_ARG,
    N_DECL, N_IDENTLIST, N_IDENTLISTP, N_TYPE, N_STMT, N_FOR, N_OPTEXPR,
    N_WHILE, N_IF, N_ELSE, N_COMP, N_STMTLIST, N_EXPR, N_EXPRREST, N_RVTAIL,
    N_RVALUEP, N_COMPARE, N_MAG, N_MAGP, N_TERM, N_TERMP, N_FACTOR,
    SYM_COUNT
};

const char* symNames[SYM_COUNT] = {
    "Adadi", "Ashriya", "Harf", "Matn", "Mantiqi", "Marqazi",
    "for", "while", "Agar", "Wagarna",
    "ID", "NUM",
    "(", ")", "{", "}", ",", "::", ":=",
    "==", "<", ">", "<=", ">=", "!=", "<>",
    "+", "-", "*", "/",
    "$", "other",
    "Program", "ProgTail", "FuncRest", "ArgList", "ArgList'", "Arg",
    "Declaration", "IdentList", "IdentList'", "Type", "Stmt", "ForStmt", "OptExpr",
    "WhileStmt", "IfStmt", "ElsePart", "CompStmt", "StmtList", "Expr", "ExprRest", "RvTail",
    "Rvalue'", "Compare", "Mag", "Mag'", "Term", "Term'", "Factor"
};

struct Prod {
    int lhs;
    int len;      // 0 for an epsilon production
    int rhs[10];
};

// Index = production number (1..64). Entry 0 is an unused placeholder.
const Prod prods[] = {
    /* 0*/ {0, 0, {}},
    /* 1*/ {N_PROGRAM, 2, {T_ADADI, N_PROGTAIL}},
    /* 2*/ {N_PROGRAM, 2, {T_ASHRIYA, N_FUNCREST}},
    /* 3*/ {N_PROGRAM, 2, {T_HARF, N_FUNCREST}},
    /* 4*/ {N_PROGRAM, 2, {T_MATN, N_FUNCREST}},
    /* 5*/ {N_PROGRAM, 2, {T_MANTIQI, N_FUNCREST}},
    /* 6*/ {N_PROGTAIL, 4, {T_MARQAZI, T_LPAREN, T_RPAREN, N_COMP}},
    /* 7*/ {N_PROGTAIL, 1, {N_FUNCREST}},
    /* 8*/ {N_FUNCREST, 5, {T_ID, T_LPAREN, N_ARGLIST, T_RPAREN, N_COMP}},
    /* 9*/ {N_ARGLIST, 2, {N_ARG, N_ARGLISTP}},
    /*10*/ {N_ARGLISTP, 3, {T_COMMA, N_ARG, N_ARGLISTP}},
    /*11*/ {N_ARGLISTP, 0, {}},
    /*12*/ {N_ARG, 2, {N_TYPE, T_ID}},
    /*13*/ {N_DECL, 3, {N_TYPE, N_IDENTLIST, T_DCOLON}},
    /*14*/ {N_IDENTLIST, 2, {T_ID, N_IDENTLISTP}},
    /*15*/ {N_IDENTLISTP, 2, {T_COMMA, N_IDENTLIST}},
    /*16*/ {N_IDENTLISTP, 0, {}},
    /*17*/ {N_TYPE, 1, {T_ADADI}},
    /*18*/ {N_TYPE, 1, {T_ASHRIYA}},
    /*19*/ {N_TYPE, 1, {T_HARF}},
    /*20*/ {N_TYPE, 1, {T_MATN}},
    /*21*/ {N_TYPE, 1, {T_MANTIQI}},
    /*22*/ {N_STMT, 1, {N_FOR}},
    /*23*/ {N_STMT, 1, {N_WHILE}},
    /*24*/ {N_STMT, 2, {N_EXPR, T_DCOLON}},
    /*25*/ {N_STMT, 1, {N_IF}},
    /*26*/ {N_STMT, 1, {N_COMP}},
    /*27*/ {N_STMT, 1, {N_DECL}},
    /*28*/ {N_STMT, 1, {T_DCOLON}},
    /*29*/ {N_FOR, 9, {T_FOR, T_LPAREN, N_EXPR, T_DCOLON, N_OPTEXPR, T_DCOLON, N_OPTEXPR, T_RPAREN, N_STMT}},
    /*30*/ {N_OPTEXPR, 1, {N_EXPR}},
    /*31*/ {N_OPTEXPR, 0, {}},
    /*32*/ {N_WHILE, 5, {T_WHILE, T_LPAREN, N_EXPR, T_RPAREN, N_STMT}},
    /*33*/ {N_IF, 6, {T_AGAR, T_LPAREN, N_EXPR, T_RPAREN, N_COMP, N_ELSE}},
    /*34*/ {N_ELSE, 2, {T_WAGARNA, N_COMP}},
    /*35*/ {N_ELSE, 0, {}},
    /*36*/ {N_COMP, 3, {T_LBRACE, N_STMTLIST, T_RBRACE}},
    /*37*/ {N_STMTLIST, 2, {N_STMT, N_STMTLIST}},
    /*38*/ {N_STMTLIST, 0, {}},
    /*39*/ {N_EXPR, 2, {T_ID, N_EXPRREST}},
    /*40*/ {N_EXPR, 2, {T_NUM, N_RVTAIL}},
    /*41*/ {N_EXPR, 4, {T_LPAREN, N_EXPR, T_RPAREN, N_RVTAIL}},
    /*42*/ {N_EXPRREST, 2, {T_ASSIGN, N_EXPR}},
    /*43*/ {N_EXPRREST, 1, {N_RVTAIL}},
    /*44*/ {N_RVTAIL, 3, {N_TERMP, N_MAGP, N_RVALUEP}},
    /*45*/ {N_RVALUEP, 3, {N_COMPARE, N_MAG, N_RVALUEP}},
    /*46*/ {N_RVALUEP, 0, {}},
    /*47*/ {N_COMPARE, 1, {T_EQ}},
    /*48*/ {N_COMPARE, 1, {T_LT}},
    /*49*/ {N_COMPARE, 1, {T_GT}},
    /*50*/ {N_COMPARE, 1, {T_LE}},
    /*51*/ {N_COMPARE, 1, {T_GE}},
    /*52*/ {N_COMPARE, 1, {T_NE}},
    /*53*/ {N_COMPARE, 1, {T_DIAMOND}},
    /*54*/ {N_MAG, 2, {N_TERM, N_MAGP}},
    /*55*/ {N_MAGP, 3, {T_PLUS, N_TERM, N_MAGP}},
    /*56*/ {N_MAGP, 3, {T_MINUS, N_TERM, N_MAGP}},
    /*57*/ {N_MAGP, 0, {}},
    /*58*/ {N_TERM, 2, {N_FACTOR, N_TERMP}},
    /*59*/ {N_TERMP, 3, {T_STAR, N_FACTOR, N_TERMP}},
    /*60*/ {N_TERMP, 3, {T_SLASH, N_FACTOR, N_TERMP}},
    /*61*/ {N_TERMP, 0, {}},
    /*62*/ {N_FACTOR, 3, {T_LPAREN, N_EXPR, T_RPAREN}},
    /*63*/ {N_FACTOR, 1, {T_ID}},
    /*64*/ {N_FACTOR, 1, {T_NUM}}
};
const int NUM_PRODS = sizeof(prods) / sizeof(prods[0]);   // 65 (incl. placeholder)

// ---- FIRST / FOLLOW / parsing table storage (plain arrays) -----------------
bool nullable[SYM_COUNT];
bool firstSet[SYM_COUNT][T_COUNT];
bool followSet[SYM_COUNT][T_COUNT];
int  parseTable[SYM_COUNT][T_COUNT];   // production number, or -1 (error box: Pop / Scan)

bool isTerminal(int s) { return s < T_COUNT; }

void writeProd(ostream& os, int p) {
    os << symNames[prods[p].lhs] << " -> ";
    if (prods[p].len == 0) { os << "epsilon"; return; }
    for (int i = 0; i < prods[p].len; i++) {
        if (i) os << " ";
        os << symNames[prods[p].rhs[i]];
    }
}

// FIRST of the right-hand side of production p, and whether the whole rhs is nullable.
void firstOfRhs(int p, bool out[T_COUNT], bool& rhsNullable) {
    for (int t = 0; t < T_COUNT; t++) out[t] = false;
    rhsNullable = true;
    for (int i = 0; i < prods[p].len; i++) {
        int s = prods[p].rhs[i];
        for (int t = 0; t < T_COUNT; t++) if (firstSet[s][t]) out[t] = true;
        if (!nullable[s]) { rhsNullable = false; break; }
    }
}

// Computes nullable, FIRST, FOLLOW and the LL(1) table. Returns number of table conflicts.
int buildParserTables() {
    for (int s = 0; s < SYM_COUNT; s++) {
        nullable[s] = false;
        for (int t = 0; t < T_COUNT; t++) { firstSet[s][t] = false; followSet[s][t] = false; parseTable[s][t] = -1; }
    }
    for (int t = 0; t < T_COUNT; t++) firstSet[t][t] = true;

    bool changed = true;
    while (changed) {                                   // nullable
        changed = false;
        for (int p = 1; p < NUM_PRODS; p++) {
            if (nullable[prods[p].lhs]) continue;
            bool all = true;
            for (int i = 0; i < prods[p].len; i++) if (!nullable[prods[p].rhs[i]]) { all = false; break; }
            if (all) { nullable[prods[p].lhs] = true; changed = true; }
        }
    }

    changed = true;
    while (changed) {                                   // FIRST
        changed = false;
        for (int p = 1; p < NUM_PRODS; p++) {
            int A = prods[p].lhs;
            for (int i = 0; i < prods[p].len; i++) {
                int Y = prods[p].rhs[i];
                for (int t = 0; t < T_COUNT; t++)
                    if (firstSet[Y][t] && !firstSet[A][t]) { firstSet[A][t] = true; changed = true; }
                if (!nullable[Y]) break;
            }
        }
    }

    followSet[N_PROGRAM][T_END] = true;                 // FOLLOW
    changed = true;
    while (changed) {
        changed = false;
        for (int p = 1; p < NUM_PRODS; p++) {
            for (int i = 0; i < prods[p].len; i++) {
                int B = prods[p].rhs[i];
                if (isTerminal(B)) continue;
                bool restNullable = true;
                for (int j = i + 1; j < prods[p].len; j++) {
                    int Y = prods[p].rhs[j];
                    for (int t = 0; t < T_COUNT; t++)
                        if (firstSet[Y][t] && !followSet[B][t]) { followSet[B][t] = true; changed = true; }
                    if (!nullable[Y]) { restNullable = false; break; }
                }
                if (restNullable) {
                    int A = prods[p].lhs;
                    for (int t = 0; t < T_COUNT; t++)
                        if (followSet[A][t] && !followSet[B][t]) { followSet[B][t] = true; changed = true; }
                }
            }
        }
    }

    int conflicts = 0;                                  // parsing table
    for (int p = 1; p < NUM_PRODS; p++) {
        bool f[T_COUNT];
        bool rhsNullable;
        firstOfRhs(p, f, rhsNullable);
        int A = prods[p].lhs;
        for (int t = 0; t < T_COUNT; t++) {
            bool put = f[t] || (rhsNullable && followSet[A][t]);
            if (!put) continue;
            if (parseTable[A][t] != -1 && parseTable[A][t] != p) conflicts++;
            else parseTable[A][t] = p;
        }
    }
    return conflicts;
}

// An empty box is POP when the terminal is in FOLLOW(A) (or is $), otherwise SCAN.
bool isPopBox(int A, int t) { return followSet[A][t] || t == T_END; }

// Writes productions, FIRST, FOLLOW and Table M (same layout as Part E) to ParsingTable.txt
void dumpTables(int conflicts) {
    ofstream out("ParsingTable.txt");
    out << "=== LL(1) PRODUCTIONS (numbers used in the table) ===\n";
    for (int p = 1; p < NUM_PRODS; p++) { out << p << ": "; writeProd(out, p); out << "\n"; }

    out << "\n=== FIRST SETS ===\n";
    for (int s = T_COUNT; s < SYM_COUNT; s++) {
        out << "FIRST(" << symNames[s] << ") = { ";
        bool first = true;
        for (int t = 0; t < T_COUNT; t++) if (firstSet[s][t]) { if (!first) out << ", "; out << symNames[t]; first = false; }
        if (nullable[s]) { if (!first) out << ", "; out << "epsilon"; }
        out << " }\n";
    }

    out << "\n=== FOLLOW SETS ===\n";
    for (int s = T_COUNT; s < SYM_COUNT; s++) {
        out << "FOLLOW(" << symNames[s] << ") = { ";
        bool first = true;
        for (int t = 0; t < T_COUNT; t++) if (followSet[s][t]) { if (!first) out << ", "; out << symNames[t]; first = false; }
        out << " }\n";
    }

    out << "\n=== TABLE M (n = production, Pop, Scan) ===\n";
    out.setf(ios::left);
    out.width(12); out << "M";
    for (int t = 0; t < T_UNKNOWN; t++) { out.width(9); out << symNames[t]; }
    out << "\n";
    for (int s = T_COUNT; s < SYM_COUNT; s++) {
        out.width(12); out << symNames[s];
        for (int t = 0; t < T_UNKNOWN; t++) {
            out.width(9);
            if (parseTable[s][t] >= 0) out << parseTable[s][t];
            else out << (isPopBox(s, t) ? "Pop" : "Scan");
        }
        out << "\n";
    }
    out << "(column 'other' = any token outside the grammar: Scan in every row)\n";

    out << "\n=== TABLE M, productions only: M[NonTerminal, Terminal] = n ===\n";
    for (int s = T_COUNT; s < SYM_COUNT; s++) {
        for (int t = 0; t < T_COUNT; t++) {
            if (parseTable[s][t] >= 0) {
                out << "M[" << symNames[s] << "," << symNames[t] << "] = " << parseTable[s][t] << ": ";
                writeProd(out, parseTable[s][t]);
                out << "\n";
            }
        }
    }
    out << "\nTable conflicts: " << conflicts << (conflicts == 0 ? "  (grammar is LL(1))" : "  (NOT LL(1)!)") << "\n";
    out.close();
}

// ---- custom stack ------------------------------------------------------------
struct Stack {
    int* data;
    int top;       // index of top element, -1 when empty
    int capacity;

    Stack() { capacity = 32; top = -1; data = new int[capacity]; }
    ~Stack() { delete[] data; }

    void push(int v) {
        if (top + 1 >= capacity) {
            capacity *= 2;
            int* nd = new int[capacity];
            for (int i = 0; i <= top; i++) nd[i] = data[i];
            delete[] data;
            data = nd;
        }
        data[++top] = v;
    }
    int pop() { return data[top--]; }
    int peek() const { return data[top]; }
};

// ---- token list (custom dynamic array) --------------------------------------
struct Token {
    int term;        // grammar terminal
    char* kind;      // lexer token class (KEYWORD, IDENTIFIER, ...)
    char* lexeme;    // lexeme text
    int line;        // start position in the source (0 = unknown)
    int col;
};

Token* toks = 0;
int numToks = 0;
int tokCap = 0;

void addToken(int term, const char* kind, const char* lexeme, int line, int col) {
    if (numToks >= tokCap) {
        int newCap = tokCap == 0 ? 64 : tokCap * 2;
        Token* nt = new Token[newCap];
        for (int i = 0; i < numToks; i++) nt[i] = toks[i];
        delete[] toks;
        toks = nt;
        tokCap = newCap;
    }
    toks[numToks].term = term;
    toks[numToks].kind = my_strdup(kind);
    toks[numToks].lexeme = my_strdup(lexeme);
    toks[numToks].line = line;
    toks[numToks].col = col;
    numToks++;
}

void freeTokens() {
    for (int i = 0; i < numToks; i++) { delete[] toks[i].kind; delete[] toks[i].lexeme; }
    delete[] toks;
    toks = 0; numToks = 0; tokCap = 0;
}

struct LexMap { const char* text; int term; };
const LexMap lexMap[] = {
    {"Adadi", T_ADADI}, {"Ashriya", T_ASHRIYA}, {"Harf", T_HARF}, {"Matn", T_MATN},
    {"Mantiqi", T_MANTIQI}, {"Marqazi", T_MARQAZI}, {"for", T_FOR}, {"while", T_WHILE},
    {"Agar", T_AGAR}, {"Wagarna", T_WAGARNA},
    {"(", T_LPAREN}, {")", T_RPAREN}, {"{", T_LBRACE}, {"}", T_RBRACE},
    {",", T_COMMA}, {"::", T_DCOLON}, {":=", T_ASSIGN},
    {"==", T_EQ}, {"<", T_LT}, {">", T_GT}, {"<=", T_LE}, {">=", T_GE},
    {"!=", T_NE}, {"<>", T_DIAMOND},
    {"+", T_PLUS}, {"-", T_MINUS}, {"*", T_STAR}, {"/", T_SLASH}
};
const int NUM_LEXMAP = sizeof(lexMap) / sizeof(lexMap[0]);

// Token-to-terminal mapping (Phase 1 token class -> grammar terminal)
int mapToken(const char* kind, const char* lexeme) {
    if (my_strcmp(kind, "IDENTIFIER")) return T_ID;
    if (my_strcmp(kind, "INTEGER") || my_strcmp(kind, "FLOAT") || my_strcmp(kind, "EXPONENTIAL")) return T_NUM;
    if (my_strcmp(kind, "KEYWORD") || my_strcmp(kind, "OPERATOR") || my_strcmp(kind, "PUNCTUATION")) {
        for (int i = 0; i < NUM_LEXMAP; i++)
            if (my_strcmp(lexeme, lexMap[i].text)) return lexMap[i].term;
    }
    return T_UNKNOWN;   // other keywords (Wapas, True ...), [ ], ++ += << ..., quoted literals
}

// Reads Token.txt ("< KIND, lexeme >") plus TokenPos.txt ("line col"), appends '$'.
bool loadTokens(const char* path, const char* posPath) {
    ifstream in(path);
    if (!in.is_open()) return false;
    ifstream posIn(posPath);
    char line[4096];
    while (in.getline(line, sizeof(line))) {
        int len = my_strlen(line);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) line[--len] = '\0';
        if (len < 6 || line[0] != '<') continue;
        int comma = -1;
        for (int i = 2; i < len; i++) if (line[i] == ',') { comma = i; break; }
        if (comma < 0) continue;
        char kind[32];
        int k = 0;
        for (int i = 2; i < comma && k < 31; i++) kind[k++] = line[i];
        kind[k] = '\0';
        char lex[4096];
        int l = 0;
        for (int i = comma + 2; i <= len - 3 && l < 4095; i++) lex[l++] = line[i];
        lex[l] = '\0';
        int tl = 0, tc = 0;
        if (posIn.is_open()) { if (!(posIn >> tl >> tc)) { tl = 0; tc = 0; } }
        addToken(mapToken(kind, lex), kind, lex, tl, tc);
    }
    addToken(T_END, "END", "$", 0, 0);
    in.close();
    return true;
}

// ---- trace / error output helpers --------------------------------------------
void writeStack(ostream& os, const Stack& st) {
    for (int i = st.top; i >= 0; i--) {
        os << symNames[st.data[i]];
        if (i > 0) os << " ";
    }
}

// Input column: terminal names (ID, NUM, ...); tokens outside the grammar show their lexeme.
void writeInput(ostream& os, int pos) {
    for (int i = pos; i < numToks; i++) {
        if (toks[i].term == T_UNKNOWN) os << toks[i].lexeme;
        else os << symNames[toks[i].term];
        if (i + 1 < numToks) os << " ";
    }
}

void writeTokText(ostream& os, int pos) {
    if (toks[pos].term == T_END) os << "end of input";
    else os << "\"" << toks[pos].lexeme << "\"";
}

void writeLoc(ostream& os, int pos) {
    if (toks[pos].line > 0) os << "line " << toks[pos].line << " col " << toks[pos].col;
    else if (toks[pos].term == T_END) os << "end of input";
    else os << "token #" << (pos + 1);
}

// kind 0: empty table box, 1: terminal mismatch, 2: input left after the end of the program
void writeShortError(ostream& os, int errNo, int kind, int X, int pos) {
    os << "E" << errNo << " ";
    writeLoc(os, pos);
    os << ": ";
    if (kind == 0) { os << "unexpected "; writeTokText(os, pos); os << " in " << symNames[X]; }
    else if (kind == 1) { os << "expected \"" << symNames[X] << "\" but found "; writeTokText(os, pos); }
    else { os << "unexpected "; writeTokText(os, pos); os << " after the end of the program"; }
}

// SyntaxErrors.txt entry: short message + (for table errors) the expected set.
void logError(ostream& os, int errNo, int step, int kind, int X, int pos) {
    writeShortError(os, errNo, kind, X, pos);
    os << "   [parser step " << step << "]\n";
    if (kind == 0) {
        os << "    expected one of: ";
        bool first = true;
        for (int t = 0; t < T_UNKNOWN; t++) {
            if (parseTable[X][t] >= 0) { if (!first) os << ", "; os << symNames[t]; first = false; }
        }
        os << "\n";
    }
}

int runParser(int lexErrors) {
    if (!loadTokens("Token.txt", "TokenPos.txt")) {
        cout << "Critical Error: Could not open Token.txt" << endl;
        return 1;
    }

    ofstream trace("ParseTrace.txt");
    ofstream synFile("SyntaxErrors.txt");
    ofstream resFile("ParseResult.txt");

    trace << "Step | Stack (top left) | Input | Production / Action | Error\n";
    trace << "-----------------------------------------------------------------------\n";

    Stack st;
    st.push(T_END);
    st.push(N_PROGRAM);

    int pos = 0, step = 0, synErrors = 0, scans = 0, pops = 0;
    bool recovering = false;     // true from an error until the next successful terminal match

    while (true) {
        int X = st.peek();
        int a = toks[pos].term;
        int errPos = pos;
        int errKind = -1;        // set when a NEW error is recorded in this step
        step++;

        trace << step << " | ";
        writeStack(trace, st);
        trace << " | ";
        writeInput(trace, pos);
        trace << " | ";

        bool finished = false;

        if (X == T_END) {
            if (a == T_END) {
                trace << "accept";
                finished = true;
            } else {
                if (!recovering) { errKind = 2; recovering = true; synErrors++; logError(synFile, synErrors, step, 2, X, pos); }
                trace << "stack is $ but input remains -> SCAN " << toks[pos].lexeme;
                synFile << "    step " << step << ": SCAN " << toks[pos].lexeme << "\n";
                scans++; pos++;
            }
        }
        else if (isTerminal(X)) {
            if (X == a) {
                trace << "match " << symNames[X];
                st.pop();
                pos++;
                recovering = false;
            } else {
                if (!recovering) { errKind = 1; recovering = true; synErrors++; logError(synFile, synErrors, step, 1, X, pos); }
                if (a == T_UNKNOWN) {
                    trace << "expected " << symNames[X] << ", found other -> SCAN " << toks[pos].lexeme;
                    synFile << "    step " << step << ": SCAN " << toks[pos].lexeme << "\n";
                    scans++; pos++;
                } else {
                    trace << "expected " << symNames[X] << ", found " << symNames[a] << " -> POP " << symNames[X];
                    synFile << "    step " << step << ": POP " << symNames[X] << " (treated as missing)\n";
                    pops++; st.pop();
                }
            }
        }
        else {
            int p = parseTable[X][a];
            if (p >= 0) {
                trace << "M[" << symNames[X] << "," << symNames[a] << "] = " << p << ": ";
                writeProd(trace, p);
                st.pop();
                for (int i = prods[p].len - 1; i >= 0; i--) st.push(prods[p].rhs[i]);
            } else {
                if (!recovering) { errKind = 0; recovering = true; synErrors++; logError(synFile, synErrors, step, 0, X, pos); }
                if (isPopBox(X, a)) {
                    trace << "M[" << symNames[X] << "," << symNames[a] << "] empty, ";
                    if (a == T_END) trace << "$ is the end marker";
                    else trace << symNames[a] << " in FOLLOW(" << symNames[X] << ")";
                    trace << " -> POP " << symNames[X];
                    synFile << "    step " << step << ": POP " << symNames[X] << "\n";
                    pops++; st.pop();
                } else {
                    trace << "M[" << symNames[X] << "," << symNames[a] << "] empty, " << symNames[a]
                          << " not in FOLLOW(" << symNames[X] << ") -> SCAN " << toks[pos].lexeme;
                    synFile << "    step " << step << ": SCAN " << toks[pos].lexeme << "\n";
                    scans++; pos++;
                }
            }
        }

        trace << " | ";
        if (errKind >= 0) writeShortError(trace, synErrors, errKind, X, errPos);
        trace << "\n";

        if (finished) break;
    }

    bool accepted = (synErrors == 0 && lexErrors == 0);

    if (synErrors == 0) synFile << "No syntax errors found.\n";

    // Result line (trace file and ParseResult.txt)
    trace << "\n";
    resFile << "";
    if (accepted) {
        trace << "Result: ACCEPT - stack and input both reduced to $ with 0 errors.\n";
        resFile << "Result: ACCEPT - stack and input both reduced to $ with 0 errors.\n";
    } else {
        trace << "Result: REJECT - " << synErrors << " syntax error(s), all recovered with panic mode ("
              << scans << " SCAN, " << pops << " POP); the input is not accepted.\n";
        resFile << "Result: REJECT - " << synErrors << " syntax error(s), all recovered with panic mode ("
                << scans << " SCAN, " << pops << " POP); the input is not accepted.\n";
        if (lexErrors > 0) resFile << "Lexical errors: " << lexErrors << " (see Error.txt)\n";
    }
    resFile << "Syntax errors: " << synErrors << "\n";
    resFile << "Lexical errors: " << lexErrors << "\n";

    cout << "Parsing complete: " << (accepted ? "ACCEPTED" : "REJECTED")
         << " (" << lexErrors << " lexical error(s), " << synErrors << " syntax error(s), "
         << scans << " SCAN, " << pops << " POP)" << endl;
    cout << "Check ParseTrace.txt, SyntaxErrors.txt and ParseResult.txt" << endl;

    trace.close();
    synFile.close();
    resFile.close();
    freeTokens();
    return 0;
}

int main(int argc, char** argv) {
    const char* sourcePath = (argc > 1) ? argv[1] : "Sourcecode.txt";

    int lexErrors = 0;
    if (runLexer(sourcePath, lexErrors) != 0) return 1;

    int conflicts = buildParserTables();
    dumpTables(conflicts);
    if (conflicts != 0) {
        cout << "Warning: grammar has " << conflicts << " LL(1) table conflict(s). See ParsingTable.txt" << endl;
    }

    return runParser(lexErrors);
}
