#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include "main.h"
#include "parser.h"
#include "generator.h"
#include "simulator.h"
#include "table.h"

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

#define da_dim(name, type)  type *name = NULL;          \
                            int _qy_ ## name ## _p = 0;  \
                            int _qy_ ## name ## _max = 0
#define da_rewind(name)     _qy_ ## name ## _p = 0
#define da_redim(name)      do {if (_qy_ ## name ## _p >= _qy_ ## name ## _max) \
                                name = realloc(name, (_qy_ ## name ## _max += 32) * sizeof(name[0]));} while (0)
#define da_append(name, x)  do {da_redim(name); name[_qy_ ## name ## _p++] = x;} while (0)
#define da_len(name)        _qy_ ## name ## _p


static FILE *source_fp, *dest_fp, *code_out_fp, *code_in_fp;

static int line = 1, col = 0, the_ch = ' ';
da_dim(text, char);

bool isReal(char* str) ;

void clean_up() {
  printf("cleanup done");
  fclose(source_fp);
  fclose(dest_fp);
}

char * copy_string(char * text) {
	char* dest = (char *)malloc(sizeof(char) * (strlen(text) + 1));
	strcpy(dest, text);
	return dest;
}

/*
static void error1(int error_code, int err_line, int err_col, const char *fmt, ... ) {
    char buf[1000];
    va_list ap;


    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    printf("(%d,%d) error: %s\n", err_line, err_col, buf);
    clean_up();
    exit(error_code);
}
*/

static int next_ch() {     /* get next char from input */
    the_ch = getc(source_fp);
    ++col;
    if (the_ch == '\n') {
        ++line;
        col = 0;
    }
    return the_ch;
}

static tok_s error(int error_code, int err_line, int err_col, const char *fmt, ... ) {
    char buf[1000];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    printf("(%d,%d) error: %s\n", err_line, err_col, buf);
    next_ch();
    return (tok_s){tk_NULL, err_line, err_col, {.text=buf}};
}



static tok_s char_lit(int n, int err_line, int err_col) {   /* 'x' */
    if (the_ch == '\'')
        return error(ERROR_INVALID_TOKEN, err_line, err_col, "gettok: empty character constant");
    if (the_ch == '\\') {
        next_ch();
        if (the_ch == 'n')
            n = 10;
        else if (the_ch == '\\')
            n = '\\';
        else error(ERROR_INVALID_TOKEN, err_line, err_col, "gettok: unknown escape sequence \\%c", the_ch);
    }
    if (next_ch() != '\'')
        error(ERROR_INVALID_TOKEN, err_line, err_col, "multi-character constant");
    next_ch();
    return (tok_s){tk_Integer, err_line, err_col, {n}};
}

static tok_s div_or_cmt(int err_line, int err_col) { /* process divide or comments */
    if (the_ch != '*')
        return (tok_s){tk_Div, err_line, err_col, {0}};

    /* comment found */
    next_ch();
    for (;;) {
        if (the_ch == '*') {
            if (next_ch() == '/') {
                next_ch();
                return gettok();
            }
        } else if (the_ch == EOF)
            error(ERROR_INVALID_TOKEN, err_line, err_col, "EOF in comment");
        else
            next_ch();
    }
}

static tok_s string_lit(int start, int err_line, int err_col) { /* "st" */
    char to_append =' ';
    da_rewind(text);

    while (next_ch() != start) {
        //printf("char is %c\n", the_ch);
        if (the_ch == '\n') return error(ERROR_INVALID_TOKEN, err_line, err_col, "EOL in string");
        if (the_ch == EOF)  return error(ERROR_INVALID_TOKEN, err_line, err_col, "EOF in string");
        if ( the_ch == '\\') {
          next_ch();
          switch (the_ch) {
            case 'n':  to_append = '\n';
                       break;
            case 't':  to_append = '\t';
                       break;
            case 'r':  to_append = '\r';
                       break;
            case '"':  to_append = '"';
                       break;
            case '\\':  to_append = '\\';
                       break;
            case 'a':  to_append = '\a';
                       break;
            case 'b':  to_append = '\b';
                       break;
            default: return error(ERROR_INVALID_TOKEN, err_line, err_col, "Invalid escape sequence");
          }
        } else {
          to_append = (char)the_ch;
        }
        da_append(text, to_append);
    }
    da_append(text, '\0');

    next_ch();

    return (tok_s){tk_String, err_line, err_col, {.text=copy_string(text)}};
}

static int kwd_cmp(const void *p1, const void *p2) {
    return strcmp(*(char **)p1, *(char **)p2);
}

static TokenType get_ident_type(const char *ident) {
    static struct {
        char *s;
        TokenType sym;
    } kwds[] = {
        {"else",  tk_Else},
		{"false", tk_False},
        {"if",    tk_If},
        {"int4", tk_Int4},
        {"print", tk_Print},
        {"putc",  tk_Putc},
		{"read", tk_Read},
		{"true", tk_True},
        {"while", tk_While}
    }, *kwp;

    return (kwp = bsearch(&ident, kwds, NELEMS(kwds), sizeof(kwds[0]), kwd_cmp)) == NULL ? tk_Ident : kwp->sym;
}

static tok_s ident_or_int(int err_line, int err_col) {
    int n;
    //create variables to count period, non_digit
    int period_count = 0;
    int nondigit_count = 0;
    float f = 0;


    da_rewind(text);
    while (isalnum(the_ch) || the_ch == '_' || the_ch == '.') {
        da_append(text, (char)the_ch);
        if (!isdigit(the_ch)) {
            nondigit_count++;
        }
        if (the_ch == '.') {
            period_count++;
        }
        next_ch();
    }
    if (da_len(text) == 0)
        error(ERROR_INVALID_TOKEN, err_line, err_col, "gettok: unrecognized character (%d) '%c'\n", the_ch, the_ch);
    da_append(text, '\0');

    if (isdigit(text[0])) {
    //  if (
    // if non_digit = 0 -> integer
    // if non_digit = 1 and count_period=1 -> real number

        if (nondigit_count > 0 && !isReal(text))
            return error(ERROR_INVALID_TOKEN, err_line, err_col, "invalid number: %s\n", text);
//        if (nondigit_count == 1 && period_count == 0)
//            error(err_line, err_col, "invalid number: %s\n", text);
    // we know that it is a integer or real

    // if floating, return float token
        if (nondigit_count > 0 && isReal(text))  {
            f = strtof(text, NULL);
            return (tok_s){tk_Float, err_line, err_col, {.f=f}};
        } else {
            n = strtol(text, NULL, 0);
            if (n == INT_MAX && errno == ERANGE)
                error(ERROR_INVALID_TOKEN, err_line, err_col, "Number exceeds maximum value");
            return (tok_s){tk_Integer, err_line, err_col, {n}};
        }
    }
    return (tok_s){get_ident_type(text), err_line, err_col, {.text=copy_string(text)}};
}

static tok_s follow(int expect, TokenType ifyes, TokenType ifno, int err_line, int err_col) {   /* look ahead for '>=', etc. */
    if (the_ch == expect) {
        next_ch();
        return (tok_s){ifyes, err_line, err_col, {0}};
    }
    if (ifno == tk_EOI)
        error(ERROR_INVALID_TOKEN, err_line, err_col, "follow: unrecognized character '%c' (%d)\n", the_ch, the_ch);
    return (tok_s){ifno, err_line, err_col, {0}};
}

void line_comment() {
  do {
    next_ch();
  } while(the_ch != '\n' || the_ch != EOF);
}

void block_comment() {
  //->>
  int state = 0;
  do {
    next_ch();
    switch (the_ch) {
      case '-': state = 1;
                break;
      case '>' : if (state > 0) {
                    state++;
                }
                break;
      case EOF: return;
      default:  state = 0;
    }
  } while(state != 3);
  next_ch();
}

tok_s gettok() {            /* return the token type */
    /* skip white space */
    nexttoken: while (isspace(the_ch))
        next_ch();
    int err_line = line;
    int err_col  = col;
    switch (the_ch) {
        case '{':  next_ch(); return (tok_s){tk_Lbrace, err_line, err_col, {0}};
        case '}':  next_ch(); return (tok_s){tk_Rbrace, err_line, err_col, {0}};
        case '(':  next_ch(); return (tok_s){tk_Lparen, err_line, err_col, {0}};
        case ')':  next_ch(); return (tok_s){tk_Rparen, err_line, err_col, {0}};
        case '+':  next_ch(); return (tok_s){tk_Add, err_line, err_col, {0}};
        case '[':  next_ch(); return (tok_s){tk_Lbracket, err_line, err_col, {0}};
        case ']':  next_ch(); return (tok_s){tk_Rbracket, err_line, err_col, {0}};
        case '-':  next_ch(); return (tok_s){tk_Sub, err_line, err_col, {0}};
        case '*':  next_ch(); return (tok_s){tk_Mul, err_line, err_col, {0}};
        case '%':  next_ch(); return (tok_s){tk_Mod, err_line, err_col, {0}};
        case ';':  next_ch(); return (tok_s){tk_Semi, err_line, err_col, {0}};
	      case ':':  next_ch(); return (tok_s){tk_Colon, err_line, err_col, {0}};
        case ',':  next_ch(); return (tok_s){tk_Comma,err_line, err_col, {0}};
        case '/':  next_ch(); return div_or_cmt(err_line, err_col);
        case '\'': next_ch(); return char_lit(the_ch, err_line, err_col);
        case '<':  next_ch();
                   if (the_ch == '<'){
                     next_ch();
                     if (the_ch == '-') {
                       block_comment();
                       goto nexttoken;
                     } else {
                       error(ERROR_INVALID_TOKEN, err_line, err_col, "after <<: unrecognized character '%c' (%d)\n", the_ch, the_ch);
                     }
                   } else if (the_ch == '-') {
                	   next_ch();
                	   return (tok_s){tk_AssignStmt, err_line, err_col, {0}};
                   } else
                   return follow('=', tk_Leq, tk_Lss,    err_line, err_col);
                   break;
        case '>':  next_ch(); return follow('=', tk_Geq, tk_Gtr,    err_line, err_col);
        case '=':  next_ch(); return follow('=', tk_Eq,  tk_Assign, err_line, err_col);
        case '!':  next_ch(); return follow('=', tk_Neq, tk_Not,    err_line, err_col);
        case '~':  next_ch(); return follow('=', tk_Neq, tk_Not,    err_line, err_col);
        case '&':  next_ch(); return follow('&', tk_And, tk_And,    err_line, err_col);
	case '@':  next_ch(); return follow('@', tk_AT, tk_AT,    err_line, err_col);
	case '.':  next_ch(); return follow('.', tk_DOT, tk_DOT,    err_line, err_col);
        case '|':  next_ch(); return follow('|', tk_Or,  tk_Or,    err_line, err_col);
	      case '"' : return string_lit(the_ch, err_line, err_col);
        case EOF:  return (tok_s){tk_EOI, err_line, err_col, {0}};
    }
    if (isalpha(the_ch)) {
      return ident_or_int(err_line, err_col);
    }
    if (isdigit(the_ch)) {
      return ident_or_int(err_line, err_col);
    }

    if (the_ch =='#') {
      line_comment();
      goto nexttoken;
    }

    return error(ERROR_INVALID_TOKEN, err_line, err_col, "follow: unrecognized character '%c' (%d)\n", the_ch, the_ch);
}

node_s * parse() {
	ret_s matched = parse_exp();
  if (matched.success) {
    fprintf(dest_fp, "valid expression\n");
    printCodeTree(dest_fp, matched.tree,1);
    // create one output file and send the machine code there
  } else {
    fprintf(dest_fp, "invalid expression\n");
  }
  return matched.tree;
}

node_s * parse_statements() {
	ret_s matched = parse_statement_block();
	if (matched.success) {
    fprintf(dest_fp, "valid statement block. code tree is \n");
    printCodeTree(dest_fp, matched.tree,1);
  } else {
    fprintf(dest_fp, "invalid statement block\n");
  }
  return matched.tree;
}

void run() {    /* tokenize the given input */
    tok_s tok;
    do {
        tok = gettok();
        printToken(dest_fp, tok);
    } while (tok.tok != tk_EOI);
    if (dest_fp != stdout)
        fclose(dest_fp);
}

void printToken(FILE * dest_fp, tok_s tok) {
    fprintf(dest_fp, "%5d  %5d %.15s",
        tok.err_ln, tok.err_col,
        &"End_of_input    Op_multiply     Op_divide       Op_mod          Op_add          "
         "Op_subtract     Op_negate       Op_not          Op_less         Op_lessequal    "
         "Op_greater      Op_greaterequal Op_equal        Op_notequal     Op_assign       "
         "Op_and          Op_or           Keyword_if      Keyword_else    Keyword_while   "
         "Keyword_print   Keyword_putc    LeftParen       RightParen      LeftBrace       "
         "RightBrace      Semicolon       Comma           Identifier      Integer         "
         "String          Lbracket        Rbracket        Colon           Float           "
         "ERROR           DOT             AT              Read            Int4            "
		 "Assignment      True            False            "
     [tok.tok * 16]);
   if (tok.tok == tk_NULL)     printf("%s", tok.text);
    else if (tok.tok == tk_Integer)     fprintf(dest_fp, "  %4d",   tok.n);
    else if (tok.tok == tk_Ident)  fprintf(dest_fp, " %s",     tok.text);
    else if (tok.tok == tk_String) fprintf(dest_fp, " \"%s\"", tok.text);
    else if (tok.tok == tk_Float)  fprintf(dest_fp, "  %f",   tok.f);
    fprintf(dest_fp, "\n");
}

void init_io(FILE **fp, FILE *std, const char mode[], const char fn[]) {
    if (fn[0] == '\0')
        *fp = std;
    else if ((*fp = fopen(fn, mode)) == NULL)
        error(ERROR_FILE_NOT_FOUND, 0, 0, "Can't open %s\n", fn);
}

void init(const char file[]) {
  init_io(&source_fp, stdin,  "r",  file);
  dest_fp = stdout;
}


int main(int argc, char *argv[]) {
    init(argc > 1 ? argv[1] : "source.txt");
    node_s* parse_tree = parse_statements();
    init_io(&code_out_fp, stdout,  "w",  "generatecode.txt");
    fprintf(dest_fp, "generating code---------------------------------------\n");
    // create one output file and send the machine code there
    prt_string_tbl();
    //generate_code(dest_fp, parse_tree);

    generate_code(code_out_fp, parse_tree);
    fclose(code_out_fp);
    init_io(&code_in_fp, stdin,  "r",  "generatecode.txt");
    fprintf(dest_fp, "executing generated code---------------------------------------\n");
    load_instructions(code_in_fp);


    clean_up();

    return 0;
}





#define false 0
#define true 1
/*
 real number : [-]d+.d+Edd
*/
bool isReal(char* str)
{
    int i, len = strlen(str);
    bool hasDecimal = false;
    bool hasE = false;

    if (len == 0)
        return (false);
    for (i = 0; i < len; i++) {
        if (str[i] != '0' && str[i] != '1' && str[i] != '2'
            && str[i] != '3' && str[i] != '4' && str[i] != '5'
            && str[i] != '6' && str[i] != '7' && str[i] != '8'
            && str[i] != '9' && str[i] != '.' && str[i] != '-'
            && str[i] != 'E' && str[i] != 'e')
            return (false);
        if (str[i] == '-' && i > 0 && str[i-1] != 'e' && str[i-1] != 'E') {
          return false;
        }
        if ((str[i] == 'E' || str[i] == 'e')  && (i == 0 || i== len - 1)) {
          return false;
        }
        if (str[i] == '.') {
          if (hasDecimal || hasE) {
            return false;
          }
          hasDecimal = true;
        }
        if (str[i] == 'E' || str[i] == 'e') {
          if (hasE) {
            return false;
          }
          hasE = true;
        }

    }
    return true;
}
