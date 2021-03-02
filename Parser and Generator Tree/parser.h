#ifndef	PARSER_H

#define PARSER_H
#include "main.h"
#include <stdbool.h>

typedef enum {
    S_Term, S_Rhs, S_Expr, S_BinOp
} SymbolType;


typedef struct node_s{
    SymbolType sym;
     struct node_s *left;
     struct node_s *right;
     union {
        tok_s tok;
     };
}  node_s ;

typedef struct {
    bool success;
     node_s *tree;
} ret_s;

ret_s parse_expression();

void printCodeTree(FILE * dest_fp,  node_s *tree, int level);

#endif
