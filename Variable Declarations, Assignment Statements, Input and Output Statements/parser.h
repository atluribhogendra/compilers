#ifndef	PARSER_H

#define PARSER_H
#include "main.h"
#include <stdbool.h>

typedef enum {
    S_Term, S_Add, S_Sub, S_Mul, S_Div, S_Mod, S_Bracket, S_Print, S_Read, S_Assignment,
	S_VarDecl, S_Expression_List, S_Statement_Block,S_Statement_List, S_String,
	S_OR_EXPRESSION, S_AND_EXPRESSION,S_NOT_EXPRESSION, S_RELATIVE_EXPRESSION,
	S_IF, S_While
} NodeType;


typedef struct node_s {
    NodeType sym;
     struct node_s *expression;
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

ret_s parse_statement_block();
ret_s parse_statement_list(bool inBlock);
ret_s parse_statement();

ret_s parse_expression();

ret_s parse_exp();

ret_s parse_logical_expression();

void printCodeTree(FILE * dest_fp,  node_s *tree, int level);

#endif
