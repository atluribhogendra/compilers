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

static tok_s current_tok;

void advance_token() {
	current_tok=gettok();
	printToken(stdout, current_tok);
}

 node_s * makeNode(NodeType sym, tok_s tok,  node_s *left,  node_s *right) {
	 node_s* node = (node_s*)malloc(sizeof(node_s));
	 node->left = left;
	 node->right = right;
	 node->expression = NULL;
	 node->tok = tok;
	 node->sym = sym;
   return node;
}

 node_s * makeNode1(NodeType sym,  node_s *left,  node_s *right) {
	 node_s* node = (node_s*)malloc(sizeof(node_s));
	 node->left = left;
	 node->right = right;
	 node->expression = NULL;
	 node->sym = sym;
   return node;
}

node_s * makeNode2(NodeType sym,  node_s *expression, node_s *left,  node_s *right) {
	 node_s* node = (node_s*)malloc(sizeof(node_s));
	 node->left = left;
	 node->right = right;
	 node->expression = expression;
	 node->sym = sym;
   return node;
}

 // string-constant, int-constant | Ident, boolean constant
ret_s parse_term() {
  tok_s tok = current_tok;
  if (tok.tok == tk_Ident
      || tok.tok == tk_Integer
      || tok.tok == tk_Float
	  || tok.tok == tk_True
	  || tok.tok == tk_False
	  || tok.tok == tk_String
      ) {
	    advance_token();
        return (ret_s){true, makeNode(S_Term, tok, NULL, NULL)};
  }
  printf("parse_+term_fail\n");
  return (ret_s){false, NULL };
}

ret_s parse_bracket() {
  tok_s tok = current_tok;
  if (tok.tok == tk_Lparen) {
	    advance_token();
	    ret_s ret_expression =  parse_logical_expression();
	    if (ret_expression.success) {
	    	if (current_tok.tok == tk_Rparen) {
	    		advance_token();
	    		return (ret_s){true, makeNode(S_Bracket, tok,ret_expression.tree, NULL)};
	    	}
	    }
  } else {
	  ret_s ret_term = parse_term();
	  if (ret_term.success) {
		  return ret_term;
	  }
  }

  printf("parse_+bracket_fail\n");
  return (ret_s){false, NULL };
}

ret_s parse_factor() {
  ret_s ret_bracket =  parse_bracket();
  if (ret_bracket.success) {
	  tok_s tok = current_tok;

	    if (tok.tok == tk_Mul || tok.tok == tk_Div || tok.tok == tk_Mod) {
	  	  NodeType sym = S_Mul;
	  	  if (tok.tok == tk_Div) {
	  		  sym = S_Div;
	  	  }
	  	  if (tok.tok == tk_Mod) {
	  		  sym = S_Mod;
	  	  }
	  	  advance_token();
	  	  ret_s ret_parse_right_factor =  parse_factor();
	  	  if (ret_parse_right_factor.success) {
	  		  return (ret_s){true, makeNode1(sym, ret_bracket.tree, ret_parse_right_factor.tree)};
	  	  } else {
	  		  return (ret_s){false, NULL };
	  	  }
       } else {
   	      return ret_bracket;
       }
  }

  return (ret_s){false, NULL };
}





ret_s parse_expression() {
  ret_s ret_factor =  parse_factor();
  if (ret_factor.success) {
	  tok_s tok = current_tok;
	  if (tok.tok == tk_Add || tok.tok == tk_Sub) {
		  NodeType sym = S_Add;
	  	  if (tok.tok == tk_Sub) {
	  		  sym = S_Sub;
	  	  }
	  	  advance_token();
	  	  ret_s ret_right_expression =  parse_expression();
	  	  if (ret_right_expression.success) {
	  		  return (ret_s){true, makeNode1(sym, ret_factor.tree, ret_right_expression.tree)};
	  	  }
	  } else {
		  return ret_factor;
	  }
  }
  printf("parse_+expression\n");
  return (ret_s){false, NULL };
}

ret_s parse_exp() {
	  advance_token();
	  return parse_expression();
}

//Expression [Rel_Op Expression]
ret_s parse_relative_expression() {
	ret_s lhs = parse_expression();
	 if (lhs.success) {
		 if (current_tok.tok == tk_Lss
				 || current_tok.tok == tk_Leq
				 || current_tok.tok == tk_Gtr
				 || current_tok.tok == tk_Geq
				 || current_tok.tok == tk_Eq
				 || current_tok.tok == tk_Neq) {
			 tok_s op_tok = current_tok;
			 advance_token();
			 ret_s rhs = parse_expression();
			 if (rhs.success) {
				 //success
				 return (ret_s){true, makeNode(S_RELATIVE_EXPRESSION, op_tok, lhs.tree, rhs.tree)};
			 }
		 } else {
			 return lhs;
		 }
	 }
	return (ret_s){false, NULL };
}

//[!] Rel_Expr
ret_s parse_not_expression() {
	bool isNot = false;
	if (current_tok.tok == tk_Not) {
		advance_token();
		isNot = true;
	}
	ret_s ret_relative_exp =  parse_relative_expression();
	if (ret_relative_exp.success) {
		if (isNot) {
			return (ret_s){true, makeNode1(S_NOT_EXPRESSION, ret_relative_exp.tree, NULL)};
		} else {
			return ret_relative_exp;
		}
	}
	return (ret_s){false, NULL };
}

//Neg_Expr [& And_Expr]
ret_s parse_and_expression() {
	ret_s not_exp =  parse_not_expression();
	if (not_exp.success) {
		if (current_tok.tok == tk_And) {
			advance_token();
			ret_s ret_and = parse_and_expression();
			if (ret_and.success) {
				//success
				return (ret_s){true, makeNode1(S_AND_EXPRESSION, not_exp.tree, ret_and.tree)};
			}
		} else{
			return not_exp;
		}
	}
	 return (ret_s){false, NULL };
}



//And_Expr [| logical_expr]
ret_s parse_logical_expression() {
	ret_s ret_and =  parse_and_expression();
	if (ret_and.success) {
		if (current_tok.tok == tk_Or) {
			advance_token();
			ret_s ret_exp = parse_logical_expression();
			if (ret_exp.success) {
				//success
				return (ret_s){true, makeNode1(S_OR_EXPRESSION, ret_and.tree, ret_exp.tree)};
			}
		} else{
			return ret_and;
		}
	}
	 return (ret_s){false, NULL };
}

// expr [,expr]*
ret_s parse_expression_list() {
	printf("parse_+expression_head_start\n");
	ret_s red_expr_head = parse_logical_expression();
	 if (!red_expr_head.success) {
		 printf("parse_+expression_head_fail\n");
		 return (ret_s){false, NULL };
	 }
	tok_s tok = current_tok;
	printf("in parse_expression_list_tail_start\n");
	if (tok.tok == tk_Comma) {
		advance_token();
		ret_s ret_exp_tail = parse_expression_list();
		if (!ret_exp_tail.success) {
			printf("parse_+expression_tail_fail\n");
			return (ret_s){false, NULL };
		}
		return (ret_s){true, makeNode1(S_Expression_List, red_expr_head.tree, ret_exp_tail.tree)};
	}
	return (ret_s){true, makeNode1(S_Expression_List, red_expr_head.tree, NULL)};
}

//print ( expr, expr, ... ) ;
ret_s parse_print_statement() {
	printf("parse_+print_start\n");
	node_s* tail = NULL;
	if (current_tok.tok == tk_Lparen) {
		advance_token();
		ret_s ret_expr_head = parse_logical_expression();
		 if (!ret_expr_head.success) {
			 printf("parse_+print_head_fail\n");
			 return (ret_s){false, NULL };
		}
		if (current_tok.tok == tk_Comma) {
			advance_token();
			ret_s ret_exp_tail = parse_expression_list();
			printf("parse_+print_tail_return\n");
			 if (!ret_exp_tail.success) {
				 printf("parse_+print_tail_fail\n");
				 return (ret_s){false, NULL };

			 }
			 tail = ret_exp_tail.tree;
		}
		if(current_tok.tok == tk_Rparen) {
			advance_token();
			if (current_tok.tok == tk_Semi) {
				advance_token();
				// success
				 return (ret_s){true, makeNode1(S_Print, ret_expr_head.tree, tail)};
			}
		}
	}
	printf("parse_+print__fail\n");
	return (ret_s){false, NULL };
}

//read ( variable ) ;
ret_s parse_read_statement() {
	if (current_tok.tok == tk_Lparen) {
		advance_token();
		if (current_tok.tok == tk_Ident) {
			tok_s tok_var = current_tok;
			advance_token();
			if (current_tok.tok == tk_Rparen) {
				advance_token();
				if (current_tok.tok == tk_Semi) {
					advance_token();
					//success
					return (ret_s){true, makeNode(S_Read, tok_var, NULL, NULL)};
				}
			}
		}
	}
	return (ret_s){false, NULL };
}

//int4 variable_name ;
ret_s parse_variable_declaration() {
	printf("in parse_variable_declaration start \n");
	if (current_tok.tok == tk_Ident) {
		tok_s tok_var = current_tok;
		advance_token();
		if (current_tok.tok == tk_Semi) {
			advance_token();
			//success
			return (ret_s){true, makeNode(S_VarDecl, tok_var, NULL, NULL)};
		}
	}
	return (ret_s){false, NULL };
}

//variable <- expr ;
ret_s parse_assignment_statement() {
	tok_s tok_var = current_tok;
	advance_token();
	if (current_tok.tok == tk_AssignStmt) {
		advance_token();
		ret_s ret_expr = parse_expression();
		 if (ret_expr.success) {
			 if (current_tok.tok == tk_Semi) {
				 advance_token();
				 //success
				 return (ret_s){true, makeNode(S_Assignment, tok_var, ret_expr.tree, NULL)};
			 }
		}
	}
	return (ret_s){false, NULL };
}

//body -> statement | { statement_block }
ret_s parse_body() {
	if (current_tok.tok == tk_Lbrace) {
		advance_token();
		ret_s statement = parse_statement_list(true);
		if (statement.success) {
			if (current_tok.tok == tk_Rbrace) {
				advance_token();
				//success
				return statement;
			}
		}
	} else {
		return parse_statement();
	}

	return (ret_s){false, NULL };
}

//if (logical_expr) body [else body]
ret_s parse_if_statement() {
	if (current_tok.tok == tk_Lparen) {
		advance_token();
		ret_s logical_expression = parse_logical_expression();
		if (logical_expression.success) {
			if (current_tok.tok == tk_Rparen) {
				advance_token();
				ret_s body = parse_body();
				if (body.success) {
					if (current_tok.tok == tk_Else) {
						advance_token();
						ret_s else_body = parse_body();
						if (else_body.success) {
							//success
							return (ret_s){true, makeNode2(S_IF, logical_expression.tree, body.tree, else_body.tree)};
						}
					} else {
						advance_token();
						//success
						return (ret_s){true, makeNode2(S_IF, logical_expression.tree, body.tree, NULL)};
					}
				}
			}
		}

	}
	return (ret_s){false, NULL };
}

// while(logical_expr) body
ret_s parse_while_statement() {
	if (current_tok.tok == tk_Lparen) {
		advance_token();
		ret_s logical_expression = parse_logical_expression();
		if (logical_expression.success) {
			if (current_tok.tok == tk_Rparen) {
				advance_token();
				ret_s body = parse_body();
				//success
				return (ret_s){true, makeNode2(S_While, logical_expression.tree, body.tree, NULL)};

			}
		}
	}
	return (ret_s){false, NULL };
}


ret_s parse_statement() {
	if(current_tok.tok == tk_Print) {
		advance_token();
		// print statement
		return parse_print_statement();
	} else if (current_tok.tok == tk_Read) {
		advance_token();
		// Read statement
		return parse_read_statement();
	} else if (current_tok.tok == tk_Int4) {
		advance_token();
		// Variable Declaration
		return parse_variable_declaration();
	} else if (current_tok.tok == tk_If) {
		advance_token();
		return parse_if_statement();
	} else if (current_tok.tok == tk_While) {
		advance_token();
		return parse_while_statement();
	} else if (current_tok.tok == tk_Ident) {
		// assignment statement
		return parse_assignment_statement();
	}
	return (ret_s){false, NULL };
}


// statement list = staterment [statement]*
ret_s parse_statement_list(bool inBlock) {
	ret_s ret_stmt_head = parse_statement();
	if (ret_stmt_head.success) {
		printCodeTree(stdout, ret_stmt_head.tree, 1);

		if (current_tok.tok == tk_EOI
				|| (inBlock && current_tok.tok == tk_Rbrace)) {
			return ret_stmt_head;
		} else {
			ret_s ret_stmt_tail = parse_statement_list(inBlock);
			if (ret_stmt_tail.success) {
				//success
				return (ret_s){true, makeNode1(S_Statement_List, ret_stmt_head.tree, ret_stmt_tail.tree)};
			}
		}
	}
	return (ret_s){false, NULL };
}

ret_s parse_statement_block() {
	advance_token();
	return parse_statement_list(false);
}

void printLeadingSpace(FILE * dest_fp, int level) {
	for (int i=0; i<level; i++) {
		fprintf(dest_fp, "    ");
	}
}

void printCodeTree(FILE * dest_fp,  node_s *tree, int level) {
    if (tree == NULL) {
    	return;
    }

    if (tree-> sym == S_Term || tree->sym == S_String) {
		printLeadingSpace(dest_fp, level);
		tok_s tok = tree -> tok;
		if (tok.tok == tk_Integer)     fprintf(dest_fp, "  %4d\n",   tok.n);
		else if (tok.tok == tk_Ident)  fprintf(dest_fp, " %s\n",     tok.text);
		else if (tok.tok == tk_String) fprintf(dest_fp, " \"%s\"\n", tok.text);
		else if (tok.tok == tk_Float)  fprintf(dest_fp, "  %f\n",   tok.f);
		return;
	} else if (tree -> sym == S_Bracket) {
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s\n",   " ( ");
		printCodeTree(dest_fp, tree -> left, level + 1);
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s\n",   " ) ");
		return;
	} else if (tree -> sym == S_Print) {
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s\n",   " print ");
		printCodeTree(dest_fp, tree -> left, level + 1);
		printCodeTree(dest_fp, tree -> right, level + 1);
		return;
	} else if (tree -> sym == S_Expression_List || tree -> sym == S_Statement_List) {
		printCodeTree(dest_fp, tree -> left, level);
		printCodeTree(dest_fp, tree -> right, level);
		return;
	} else if (tree -> sym == S_Read) {
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s %s\n",   " read", tree->tok.text);
		return;
	} else if (tree -> sym == S_VarDecl) {
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s %s\n",   " int4", tree->tok.text);
		return;
	} else if (tree -> sym == S_Assignment) {
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s %s\n",   " Assign", tree->tok.text);
		printCodeTree(dest_fp, tree -> left, level + 1);
		return;
	} else if (tree -> sym == S_IF) {
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s\n",   " if");
		printCodeTree(dest_fp, tree -> expression, level);
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s\n",   " body");
		printCodeTree(dest_fp, tree -> left, level + 1);
		if (tree->right != NULL) {
			printLeadingSpace(dest_fp, level);
			fprintf(dest_fp, " %s\n",   "else-body");
			printCodeTree(dest_fp, tree -> right, level + 1);
		}
		return;
	} else if (tree -> sym == S_While) {
		printLeadingSpace(dest_fp, level);
		fprintf(dest_fp, " %s\n",   " while");
		printCodeTree(dest_fp, tree -> expression, level);
		printCodeTree(dest_fp, tree -> left, level + 1);
		return;
	}

	printCodeTree(dest_fp, tree -> left, level + 1);
	printLeadingSpace(dest_fp, level);
    switch (tree -> sym) {
    	case S_Add:
    		fprintf(dest_fp, " %s\n",   " + ");
    		break;
    	case S_Sub:
    		fprintf(dest_fp, " %s\n",   " - ");
    		break;
    	case S_Mul:
    		fprintf(dest_fp, " %s\n",   " * ");
    		break;
    	case S_Div:
    		fprintf(dest_fp, " %s\n",   " / ");
    		break;
    	case S_Mod:
    		fprintf(dest_fp, " %s\n",   " % ");
    		break;
    	case S_OR_EXPRESSION:
    		fprintf(dest_fp, " %s\n",   " | ");
    		break;
    	case S_AND_EXPRESSION:
    		fprintf(dest_fp, " %s\n",   " & ");
    		break;
    	case S_NOT_EXPRESSION:
    		fprintf(dest_fp, " %s\n",   " ! ");
    		break;
    	case S_RELATIVE_EXPRESSION: {
    		char * op = " ";


    		switch (tree->tok.tok) {
    		case tk_Lss:
    			op = "<";
    			break;
    		case tk_Leq:
    			op = "<";
    			break;
    		case tk_Eq:
    			op = "==";
    			break;
    		case tk_Neq:
    			op = "!=";
    			break;
    		case tk_Gtr:
    			op = ">";
    			break;
    		case tk_Geq:
    			op = ">=";
    			break;
    		default:
    			op = "";
    		}
    		fprintf(dest_fp, " %s\n",   op);
    	}
    		break;

    	default:
    	    break;
    };
	printCodeTree(dest_fp, tree -> right, level +1);
}
