#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "parser.h"
#include "generator.h"

//ASSUMPTIONS:
// REGISTERS: R1
const char *REGISTER1 = "R1";
const char *ACCUMULATOR = "AC";

void generateTerm(FILE * dest_fp,  node_s *tree) {
//Push the value onto stack
	fprintf(dest_fp, " %s %d\n",   "PUSH", tree->tok.n);
}

void generateBinOP(FILE * dest_fp,  node_s *tree) {
//Pop two values from stack

	fprintf(dest_fp, " %s %s\n",   "POP", REGISTER1);
	fprintf(dest_fp, " %s %s\n",   "POP", ACCUMULATOR);

//Do the operation
	if (tree -> tok.tok == tk_Add) {
		fprintf(dest_fp, " %s %s\n",   "ADD", REGISTER1);
	} else if (tree -> tok.tok == tk_Sub) {
		fprintf(dest_fp, " %s %s\n",   "SUB", REGISTER1);
	}

//Push the result into stack
	fprintf(dest_fp, " %s %s\n",   "PUSH", ACCUMULATOR);
}


void generateCode(FILE * dest_fp,  node_s *tree) {

	   if (tree == NULL) {
	    	return;
	    }

	   generateCode(dest_fp, tree -> left);
	   generateCode(dest_fp, tree -> right);
	    switch (tree -> sym) {
	    	case S_Term:
	    		generateTerm(dest_fp, tree);
	    	    break;
	    	case S_Add:
	    		fprintf(dest_fp, " %s\n",   "Rhs Code");
	    		break;
	    	case S_Mul:
	    		fprintf(dest_fp, " %s\n",   "Expr Code");
	    		break;
	    	case S_Div:
	    		generateBinOP(dest_fp, tree);
	    		break;
	    	default:
	    		break;
	    };
}

