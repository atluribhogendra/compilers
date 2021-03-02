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


 node_s * makeNode(SymbolType sym, tok_s tok,  node_s *left,  node_s *right) {
	 node_s* node = (node_s*)malloc(sizeof(node_s));
	 node->left = left;
	 node->right = right;
	 node->tok = tok;
	 node->sym = sym;
   return node;
}

 node_s * makeNode1(SymbolType sym,  node_s *left,  node_s *right) {
	 node_s* node = (node_s*)malloc(sizeof(node_s));
	 node->left = left;
	 node->right = right;
	 node->sym = sym;
   return node;

}

ret_s term() {
  tok_s tok = gettok();
  if (tok.tok == tk_Ident
      || tok.tok == tk_Integer
      || tok.tok == tk_Float
      ) {
        return (ret_s){true, makeNode(S_Term, tok, NULL, NULL)};
  }
  return (ret_s){false, NULL };
}

ret_s rhs() {

  tok_s tok = gettok();
  if (tok.tok == tk_EOI) {
    return (ret_s){true, NULL};
  }

  if (tok.tok == tk_Add) {
    ret_s ret =  parse_expression();
    if (ret.success) {
      return (ret_s){true, makeNode(S_BinOp, tok, ret.tree, NULL)};
    }
  }
  return (ret_s){false, NULL };
}

ret_s parse_expression() {
  ret_s ret_term =  term();
  if (ret_term.success) {
    ret_s ret_rhs =  rhs();
    if (ret_rhs.success) {
      return (ret_s){true, makeNode1(S_Expr, ret_term.tree, ret_rhs.tree)};
    }
  }

  return (ret_s){false, NULL };
}

void printCodeTree(FILE * dest_fp,  node_s *tree, int level) {
    if (tree == NULL) {
    	return;
    }

	printCodeTree(dest_fp, tree -> left, level + 1);

	for (int i=0; i<level; i++) {
		fprintf(dest_fp, "    ");
	}

	if (tree-> sym == S_Term) {
		//printToken(dest_fp, (tree -> tok));
	}

    switch (tree -> sym) {
    	case S_Term:
    		fprintf(dest_fp, " %s\n",   "Term");
    	    break;
    	case S_Rhs:
    		fprintf(dest_fp, " %s\n",   "Rhs");
    		break;
    	case S_Expr:
    		fprintf(dest_fp, " %s\n",   "Expr");
    		break;
    	case S_BinOp:
    		fprintf(dest_fp, " %s\n",   "BinOp + ");
    		break;
    };
	printCodeTree(dest_fp, tree -> right, level +1);
}

