#define ERROR_FILE_NOT_FOUND 2
#define ERROR_INVALID_TOKEN 3

typedef enum {
    tk_EOI, tk_Mul, tk_Div, tk_Mod, tk_Add, tk_Sub, tk_Negate, tk_Not, tk_Lss, tk_Leq,
    tk_Gtr, tk_Geq, tk_Eq, tk_Neq, tk_Assign, tk_And, tk_Or, tk_If, tk_Else, tk_While,
    tk_Print, tk_Putc, tk_Lparen, tk_Rparen, tk_Lbrace, tk_Rbrace, tk_Semi, tk_Comma,
    tk_Ident, tk_Integer, tk_String, tk_Lbracket, tk_Rbracket, tk_Colon, tk_Float,
    tk_NULL, tk_DOT, tk_AT
} TokenType;

typedef struct {
    TokenType tok;
    int err_ln, err_col;
    union {
        int n;                  /* value for integer literals */
        char *text;             /* text for idents */
        float f;                /* value for floating point literals */
    };
} tok_s;
tok_s gettok();
void clean_up();
void init(const char file[]);

