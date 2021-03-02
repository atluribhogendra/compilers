#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "parser.h"
#include "generator.h"
#include "simulator.h"

#define MAX_INSTRUCTION_LEN 3000
instruction_t instructions[3000];
int last_instruction = 0;

int add_instruction(opcode_s opcode, int operand) {
	instructions[last_instruction].opcode = opcode;
	instructions[last_instruction].operand = operand;
	return last_instruction++;
}

void update_jump(int label, int value) {
	instructions[label].operand = value - label;
}

void generateTerm(node_s *tree) {
	if (tree->tok.tok == tk_Ident) {
		//Load the memory address and push onto stack
		add_instruction(OP_LOAD, tree->ref);
		add_instruction(OP_PUSH, REG_ACC);
	} else {
		//Push the value onto stack
		int value = tree->tok.n;
		if (tree->valueType==DataType_String) {
			value = tree->ref;
		} else if (tree->tok.tok == tk_True) {
			value = 1;
		} else if (tree->tok.tok == tk_False) {
			value = 0;
		}
		add_instruction(OP_PUSH_IMMEDIATE, value);
	}
}

void generateAssignment(node_s *tree) {
	//Pop the stack and push it into the the memory address
	generate_code_tree(tree->left);
	add_instruction(OP_POP, REG_ACC);
	add_instruction( OP_STORE, tree->ref);
}

void generate_read_statement( node_s *tree) {
	add_instruction(OP_PUSH_IMMEDIATE, tree->ref);
	add_instruction(OP_CALL, F_READ_INT);
}

void generate_if_statement(node_s *tree) {
	// expression evaluation
	generate_code_tree( tree->expression);

	add_instruction(OP_POP, REG_ACC);
	int label1 = add_instruction( OP_JZ, 0);
	// true block
	generate_code_tree(tree->left);
	if (tree->right) {
		// false block
		int label2 = add_instruction(OP_JMP, 0);
		// get IP and adjust 1
		update_jump(label1, last_instruction);
		generate_code_tree(tree->right);
		update_jump(label2, last_instruction);

	} else {
		update_jump(label1, last_instruction);
	}
}

void generate_while_statement(node_s *tree) {
	//1) Generate a non-conditional jump to jump to the location of the decision.
	int label1 = add_instruction( OP_JMP, 0);
	// 2) Generate the code of the loop body.
	generate_code_tree(tree->left);

	// 3) Adjust the jump in 1) to jump over the code of 2).
	update_jump(label1, last_instruction);

	//4) Generate the code of the decision expression.
	generate_code_tree( tree->expression);

	//5) Generate an appropriate TST instruction.
	//6) Generate a conditional jump. The jump should happen if the result of the expression is true.
	//The jump should go backward over the code of 6) 5) 4) and 2) to the start of the loop body in 2).
	add_instruction(OP_POP, REG_ACC);
	add_instruction( OP_JNZ,label1-last_instruction+1);
}

void generateBinOP(node_s *tree, opcode_s op) {
	//Pop two values from stack

	add_instruction(OP_POP, REG_ACC);
	add_instruction(OP_POP, REG_R1);

	//Do the operation
	add_instruction(op, REG_R1);

	//Push the result into stack
	add_instruction(OP_PUSH, REG_ACC);
}


void generate_or(node_s *tree) {
	// expression evaluation
	generate_code_tree( tree->left);
	add_instruction(OP_POP, REG_ACC);
	add_instruction( OP_JZ, 3);
	add_instruction(OP_PUSH_IMMEDIATE, 1);
	int label1 = add_instruction( OP_JMP, 0);
	// right operand
	generate_code_tree(tree->right);
	update_jump(label1, last_instruction);
}

void generate_and(node_s *tree) {
	// expression evaluation
	generate_code_tree( tree->left);
	add_instruction(OP_POP, REG_ACC);
	add_instruction( OP_JNZ, 3);
	add_instruction(OP_PUSH_IMMEDIATE, 0);
	int label1 = add_instruction( OP_JMP, 0);
	// right operand
	generate_code_tree(tree->right);
	update_jump(label1, last_instruction);
}

void generate_not(node_s *tree) {
	// expression evaluation
	generate_code_tree( tree->left);
	add_instruction(OP_POP, REG_ACC);
	add_instruction( OP_JNZ, 3);
	add_instruction(OP_PUSH_IMMEDIATE, 1);
	add_instruction( OP_JMP, 2);
	add_instruction(OP_PUSH_IMMEDIATE, 0);
}

//print ( expr, expr, ... ) ;
// print expr.head expr.tail
// generate code for Expr.ead
// genrate code to call print function
// generate code to print tail
void generatePrint( node_s *tree) {

	// generate code for head expression
	generate_code_tree( tree->left);


	//Call function to print INT4 or String
	function_s function = F_NONE;
	switch(tree->left->valueType) {
	case DataType_INT4:
		function = F_PRINT_INT;
		break;
	case DataType_String:
		function = F_PRINT_STR;
		break;
	case DataType_Boolean:
		function = F_PRINT_BOOL;
		break;
	default:
		// cannot print null data type

		break;
	}
	add_instruction(OP_CALL, function);

	// generate code to print tail
	if (tree->right != NULL) {
		generatePrint(tree->right);
	}
}


void generate_code_tree(node_s *tree) {

	if (tree == NULL) {
		return;
	}

	switch (tree -> sym) {
	case S_Print:
		generatePrint(tree);
		return;
	case S_VarDecl:
		add_instruction(OP_CLEARMEM, tree->ref);
		return;
	case S_Assignment:
		generateAssignment(tree);
		return;
	case S_Read:
		generate_read_statement(tree);
		return;
	case S_IF:
		generate_if_statement( tree);
		return;

	case S_While:
		generate_while_statement( tree);
		return;

	case S_OR_EXPRESSION:
		generate_or(tree);
		return;
	case S_AND_EXPRESSION:
		generate_and(tree);
		return;

	case S_NOT_EXPRESSION:
		generate_not(tree);
		return;

	default:
		break;

	}

	generate_code_tree(tree -> left);
	generate_code_tree(tree -> right);
	switch (tree -> sym) {

	case S_Term:
		generateTerm(tree);
		break;
	case S_Add:
		generateBinOP(tree, OP_ADD);
		break;
	case S_Sub:
		generateBinOP(tree, OP_SUB);
		break;
	case S_Mul:
		generateBinOP(tree, OP_MUL);
		break;
	case S_Div:
		generateBinOP(tree, OP_DIV);
		break;

	case S_Mod:
		generateBinOP(tree, OP_MOD);
		break;

	case S_RELATIVE_EXPRESSION: {
		switch (tree->tok.tok) {

		case tk_Lss:
			generateBinOP(tree, OP_CMPLT);
			break;

		case tk_Leq:
			generateBinOP(tree, OP_CMPLE);
			break;

		case tk_Eq:
			generateBinOP(tree, OP_CMPEQ);
			break;

		case tk_Neq:
			generateBinOP(tree, OP_CMPNE);
			break;

		case tk_Gtr:
			generateBinOP(tree, OP_CMPGT);
			break;

		case tk_Geq:
			generateBinOP(tree, OP_CMPGE);
			break;

		default:
			break;
		}

		default:
			break;
	};
	}
}

void generate_code(FILE * dest_fp,  node_s *tree){
    generate_code_tree(tree);
    add_instruction(OP_HALT,0);
    for (int i=0; i<last_instruction; i++) {
    	instruction_t instruction = instructions[i];
    	fprintf(dest_fp, "%d %d\n", instruction.opcode, instruction.operand);
    }
}

