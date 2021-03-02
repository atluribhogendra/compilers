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
#include "table.h"

static tok_s current_tok;

ret_s parse_error(const char *error_message) {
    printf("(%d,%d) parser error: %s\n", current_tok.err_ln, current_tok.err_col, error_message);
    return (ret_s){false, NULL};
}

void advance_token() {
	current_tok=gettok();
	//printToken(stdout, current_tok);
}

 node_s * makeNode(NodeType sym, tok_s tok,  node_s *left,  node_s *right, DataType valueType) {
	 node_s* node = (node_s*)malloc(sizeof(node_s));
	 node->left = left;
	 node->right = right;
	 node->expression = NULL;
	 node->tok = tok;
	 node->sym = sym;
	 node->valueType = valueType;
   return node;
}

 node_s * makeNode1(NodeType sym,  node_s *left,  node_s *right, DataType valueType) {
	 node_s* node = (node_s*)malloc(sizeof(node_s));
	 node->left = left;
	 node->right = right;
	 node->expression = NULL;
	 node->sym = sym;
	 node->valueType = valueType;
   return node;
}

node_s * makeNode2(NodeType sym,  node_s *expression, node_s *left,  node_s *right) {
	 node_s* node = (node_s*)malloc(sizeof(node_s));
	 node->left = left;
	 node->right = right;
	 node->expression = expression;
	 node->sym = sym;
	 node->valueType = DataType_NULL;
   return node;
}

 // string-constant, int-constant | Ident, boolean constant
ret_s parse_term() {
	DataType valueType;
	symtab_ele* element = NULL;
  switch (current_tok.tok) {
  case tk_Ident:
	  valueType = DataType_INT4;
	  if ((element = find_symbol(current_tok.text, DataType_INT4)) == NULL)
	  		return parse_error("Undefined Variable.");
	  break;
  case tk_Integer:
	  valueType = DataType_INT4;
	  break;
  //case tk_Float:
  case tk_True:
	  valueType = DataType_Boolean;
	  break;
  case tk_False:
	  valueType = DataType_Boolean;
	  break;
  case tk_String:
	  valueType = DataType_String;
	  break;
  default:
	  return parse_error("Parse term failed. Expected Identifier, Integer, true, False or String.");
  }
  tok_s tok = current_tok;
  advance_token();

  ret_s ret_term = (ret_s){true, makeNode(S_Term, tok, NULL, NULL, valueType)};
  if (element !=NULL) {
	  ret_term.tree->ref = element->memory_address;
  } else if (valueType == DataType_String) {
	  int index = insert_string(tok.text);
	  ret_term.tree->ref = index;
  }

  return ret_term;
}

ret_s parse_bracket() {
  tok_s tok = current_tok;
  if (tok.tok == tk_Lparen) {
	    advance_token();
	    ret_s ret_expression =  parse_logical_expression();
	    if (ret_expression.success) {
	    	if (current_tok.tok == tk_Rparen) {
	    		advance_token();
	    		return (ret_s){true, makeNode(S_Bracket, tok,ret_expression.tree, NULL,ret_expression.tree->valueType)};
	    	}
	    }
  } else {
	  ret_s ret_term = parse_term();
	  if (ret_term.success) {
		  return ret_term;
	  }
  }

  return parse_error("Parse bracket failed.");
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
	  		  // semantic check - both operands must be int4
	  		  if (ret_bracket.tree->valueType == DataType_INT4
	  				  && ret_parse_right_factor.tree->valueType == DataType_INT4) {
		  		  return (ret_s){true, makeNode1(sym, ret_bracket.tree, ret_parse_right_factor.tree, DataType_INT4)};
	  		  } else {
	  			return parse_error("Both Operands should be of type int4");
	  		  }

	  	  } else {
	  		return parse_error("Parse Right Hand Factor failed.");
	  	  }
       } else {
   	      return ret_bracket;
       }
  }
  return parse_error("Parse Factor failed.");
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
				if(ret_factor.tree->valueType == DataType_INT4
						&& ret_right_expression.tree->valueType == DataType_INT4) {
					return (ret_s){true, makeNode1(sym, ret_factor.tree, ret_right_expression.tree,DataType_INT4)};
				} else {
					return parse_error("Both Operands should be of type int4");
				}
			} else {
				return parse_error("Parse Right Hand Expression failed.");
			}
		} else {
			return ret_factor;
		}
	}
	return parse_error("Parse Expression failed.");
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
				if(lhs.tree->valueType == DataType_INT4
						&& rhs.tree->valueType == DataType_INT4) {
					return (ret_s){true, makeNode(S_RELATIVE_EXPRESSION, op_tok, lhs.tree, rhs.tree,DataType_Boolean)};
				} else {
					return parse_error("Both Operands should be of type INT4");
				}
			} else {
				return parse_error("Parse Right Hand Relative Expression failed.");
			}
		} else {
			return lhs;
		}
	}
	return parse_error("Parse Relative Expression failed.");
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
			if (ret_relative_exp.tree->valueType == DataType_Boolean) {
				return (ret_s){true, makeNode1(S_NOT_EXPRESSION, ret_relative_exp.tree, NULL, DataType_Boolean)};
			} {
				return parse_error("Operand of Not operator should be of type Boolean");
			}

		} else {
			return ret_relative_exp;
		}
	}
	return parse_error("Parse Not Expression failed.");
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
				if (not_exp.tree->valueType == DataType_Boolean
						&& ret_and.tree->valueType == DataType_Boolean) {
					return (ret_s){true, makeNode1(S_AND_EXPRESSION, not_exp.tree, ret_and.tree, DataType_Boolean)};
				} else {
					return parse_error("Both Operands should be of type Boolean");
				}
			} else {
				return parse_error("Parse Right Hand And Expression failed.");
			}
		} else{
			return not_exp;
		}
	}
	return parse_error("Parse And Expression failed.");
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
				if (ret_and.tree->valueType == DataType_Boolean
						&& ret_exp.tree->valueType == DataType_Boolean) {
					return (ret_s){true, makeNode1(S_OR_EXPRESSION, ret_and.tree, ret_exp.tree, DataType_Boolean)};
				} else {
					return parse_error("Both Operands should be of type Boolean");
				}
			} else {
				return parse_error("Parse Right Hand Or Expression failed.");
			}
		} else{
			return ret_and;
		}
	}
	return parse_error("Parse Logical Expression failed.");
}

// expr [,expr]*
ret_s parse_expression_list() {
	ret_s red_expr_head = parse_logical_expression();
	 if (!red_expr_head.success) {
		 return parse_error("Parse Expression Head failed.");
	 }
	tok_s tok = current_tok;
	if (tok.tok == tk_Comma) {
		advance_token();
		ret_s ret_exp_tail = parse_expression_list();
		if (!ret_exp_tail.success) {
			return parse_error("Parse Expression Tail failed.");
		}
		return (ret_s){true, makeNode1(S_Expression_List, red_expr_head.tree, ret_exp_tail.tree,DataType_NULL)};
	}
	return (ret_s){true, makeNode1(S_Expression_List, red_expr_head.tree, NULL,DataType_NULL)};
}

//print ( expr, expr, ... ) ;
ret_s parse_print_statement() {
	node_s* tail = NULL;
	if (current_tok.tok == tk_Lparen) {
		advance_token();
		ret_s ret_expr_head = parse_logical_expression();
		 if (!ret_expr_head.success) {
			 return parse_error("Parse Print Head failed.");
		}
		if (current_tok.tok == tk_Comma) {
			advance_token();
			ret_s ret_exp_tail = parse_expression_list();
			 if (!ret_exp_tail.success) {
				 return parse_error("Parse Expression Tail Failed.");

			 }
			 tail = ret_exp_tail.tree;
		}
		if(current_tok.tok == tk_Rparen) {
			advance_token();
			if (current_tok.tok == tk_Semi) {
				advance_token();
				// success
				 return (ret_s){true, makeNode1(S_Print, ret_expr_head.tree, tail,DataType_NULL)};
			}
		}
	}
	return parse_error("Parse Print Failed.");
}

//read ( variable ) ;
ret_s parse_read_statement() {
	symtab_ele * element = NULL;
	if (current_tok.tok == tk_Lparen) {
		advance_token();
		if (current_tok.tok == tk_Ident) {
			tok_s tok_var = current_tok;
			advance_token();
			if (current_tok.tok == tk_Rparen) {
				advance_token();
				if (current_tok.tok == tk_Semi) {
					advance_token();
					if ((element = find_symbol(tok_var.text, DataType_INT4)) == NULL)
						return parse_error("Undefined Variable.");
					//success
					ret_s ret_read = (ret_s){true, makeNode(S_Read, tok_var, NULL, NULL,DataType_NULL)};
					ret_read.tree->ref = element->memory_address;
					return ret_read;
				}
			}
		}
	}
	return parse_error("Parse Read Failed.");
}

//int4 variable_name ;
ret_s parse_variable_declaration() {
	symtab_ele * element = NULL;
	if (current_tok.tok == tk_Ident) {
		tok_s tok_var = current_tok;
		advance_token();
		if (current_tok.tok == tk_Semi) {
			advance_token();
			// check if variable is already defined.
			if (find_symbol(tok_var.text, DataType_INT4) != NULL) {
				return parse_error("Duplicate Variable Declaration.");
			}
			//success
			// insert variable in symbol table
			element = insert_symbol(tok_var.text, DataType_INT4);
			ret_s ret_var_declaration= (ret_s){true, makeNode(S_VarDecl, tok_var, NULL, NULL,DataType_NULL)};
			ret_var_declaration.tree->ref = element->memory_address;
			return ret_var_declaration;
		}
	}
	return parse_error("Parse Variable Declaration Failed.");
}

//variable <- expr ;
ret_s parse_assignment_statement() {
	symtab_ele * element = NULL;
	tok_s tok_var = current_tok;
	advance_token();
	if (current_tok.tok == tk_AssignStmt) {
		advance_token();
		ret_s ret_expr = parse_expression();
		 if (ret_expr.success) {
			 if (current_tok.tok == tk_Semi) {
				 advance_token();
				  if ((element = find_symbol(tok_var.text, DataType_INT4)) == NULL)
				  		return parse_error("Undefined Variable.");

				 //success
				  ret_s ret_assignment = (ret_s){true, makeNode(S_Assignment, tok_var, ret_expr.tree, NULL,DataType_NULL)};
				  ret_assignment.tree->ref = element->memory_address;
				 return ret_assignment;
			 }
		}
	}
	return parse_error("Parse Assignment Statement Failed.");
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

	return parse_error("Parse Body Failed.");
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
							if (logical_expression.tree->valueType == DataType_Boolean) {
								return (ret_s){true, makeNode2(S_IF, logical_expression.tree, body.tree, else_body.tree)};
							}
							else{
								return parse_error("expression for if should be of type Boolean.");
							}
						} else {
							return parse_error("Parse else body failed.");
						}
					} else {
						advance_token();
						//success
						if (logical_expression.tree->valueType == DataType_Boolean) {
							return (ret_s){true, makeNode2(S_IF, logical_expression.tree, body.tree, NULL)};
						} else{
							return parse_error("expression for if should be of type Boolean.");
						}
					}
				}
			}
		}

	}
	return parse_error("Parse If Statement Failed.");
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
				if (logical_expression.tree->valueType == DataType_Boolean) {
					return (ret_s){true, makeNode2(S_While, logical_expression.tree, body.tree, NULL)};
				} else{
					return parse_error("expression for While condition should be of type Boolean.");
				}
			}
		}
	}
	return parse_error("Parse While Statement Failed.");
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
	return parse_error("Parse Statement Failed.");
}


// statement list = staterment [statement]*
ret_s parse_statement_list(bool inBlock) {
	ret_s ret_stmt_head = parse_statement();
	if (ret_stmt_head.success) {
		//printCodeTree(stdout, ret_stmt_head.tree, 1);

		if (current_tok.tok == tk_EOI
				|| (inBlock && current_tok.tok == tk_Rbrace)) {
			return ret_stmt_head;
		} else {
			ret_s ret_stmt_tail = parse_statement_list(inBlock);
			if (ret_stmt_tail.success) {
				//success
				return (ret_s){true, makeNode1(S_Statement_List, ret_stmt_head.tree, ret_stmt_tail.tree,DataType_NULL)};
			}
		}
	}
	return parse_error("Parse Statement List Failed.");
}

ret_s parse_statement_block() {
	init_string_tbl();
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
		if (tree->valueType == DataType_String)  fprintf(dest_fp, "  string#%d\n", tree->ref);
		else if (tok.tok == tk_Integer)     fprintf(dest_fp, "  %4d\n",   tok.n);
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
