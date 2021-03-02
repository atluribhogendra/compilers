/*
 * simulator.c
 *
 *  Read Code File instructions line by line
 *  Execute the instructions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simulator.h"
#include "table.h"

#define OPERAND_STACK_LEN 1024
#define MAX_CODE_LEN 3000
#define MAX_DATA_LEN 1000


#define TRACE_SIMULATOR 0

char * opcodes[] = {"OP_PUSH", "OP_PUSH_IMMEDIATE","OP_LOAD", "OP_STORE", "OP_CLEARMEM", "OP_POP",
		"OP_ADD", "OP_SUB","OP_MUL", "OP_DIV", "OP_MOD", "OP_CALL", "OP_JZ", "OP_JMP", "OP_JNZ", "OP_HALT",
		  "OP_CMPLT" };

 char* opcode_to_text(opcode_s opcode) {

	return opcodes[opcode];
}

int operand_stack[OPERAND_STACK_LEN];
int operand_stack_last = 0;

instruction_t code_arr[MAX_CODE_LEN];
int  code_arr_last= 0;


int data_arr[MAX_DATA_LEN];

void push_operand(int value) {
	operand_stack[operand_stack_last++] = value;
}

int pop_operand() {
	int value = operand_stack[--operand_stack_last];
	return value;
}

void print_stack() {
	printf("Stack:");
	for(int i= operand_stack_last - 1; i >= 0; i--) {
		printf("%d,", operand_stack[i]);
	}
	printf("\n");
}

int registers[6];

int read_register(int reg) {
	int value = registers[reg];
	return value;
}

void write_register(int reg, int value) {
	registers[reg] = value;
}

void print_registers() {
	for (int i=0; i <= REG_IP; i++) {
		printf("%d ", registers[i]);
	}
	printf("\n");
}

void execute_call(function_s fun) {
	//printf("Execute_print\n");
	int value = pop_operand();
	switch (fun) {
	case F_PRINT_INT:
		printf("%d\n", value);
		break;
	case F_PRINT_STR:
		printf("%s\n", get_string(value));
		break;
	case F_PRINT_BOOL:
		printf("%s\n", value == 0 ? "false":"true");
		break;
	case F_READ_INT: {
		printf("Enter Input>\n");
		char line1[128];
		fgets ( line1, sizeof(line1), stdin);
		printf("%s\n", line1);
		int input;
		input = -1;
		//scanf("%d", &input);
		data_arr[value] = input;
	}
	break;
	default:
		break;
	}
}

void trace_execution() {

	if (TRACE_SIMULATOR) {
		printf("------------\n\n");
		print_registers();
		print_stack();
	}

}


void execute() {
	instruction_t* instruction;
	int next_ip = 0;

	while(1) {

		//FETCH Instruction
		int ip = read_register(REG_IP);
		instruction = &code_arr[ip];
		next_ip = ip + 1;
		int operand = instruction->operand;

		trace_execution();
		if(TRACE_SIMULATOR)
		    printf("%s %d\n", opcode_to_text(instruction->opcode), operand);

		// EXEUTE

		//OP_JZ, OP_JMP, OP_JNZ, OP_HALT
		switch (instruction->opcode) {
		case OP_PUSH:
			push_operand(read_register(operand));
			break;
		case OP_PUSH_IMMEDIATE:
			push_operand(operand);
			break;
		case OP_POP:
			write_register(operand, pop_operand());
			break;
		case OP_ADD:
			write_register(REG_ACC, read_register(operand) + read_register(REG_ACC));
			break;
		case OP_SUB:
			write_register(REG_ACC, read_register(operand) - read_register(REG_ACC));
			break;
		case OP_MUL:
			write_register(REG_ACC, read_register(operand) * read_register(REG_ACC));
			break;
		case OP_DIV:
			write_register(REG_ACC, read_register(operand) / read_register(REG_ACC));
			break;
		case OP_MOD:
			write_register(REG_ACC, read_register(operand) % read_register(REG_ACC));
			break;
		case OP_CALL:
			execute_call(operand);
			break;
		case OP_LOAD:
			write_register(REG_ACC, data_arr[operand]);
			break;

		case OP_STORE:
			data_arr[operand] = read_register(REG_ACC);
			break;

		case OP_CLEARMEM:
			data_arr[operand] = 0;
			break;

		case OP_JZ:
			if (read_register(REG_ACC) == 0) {
				next_ip = ip + operand;
			}
			break;

		case OP_JMP:
			next_ip = ip + operand;
			break;

		case OP_JNZ:
			if (read_register(REG_ACC) != 0) {
				next_ip = ip + operand;
			}
			break;

		case OP_HALT:
			return;


		case OP_CMPLT:
			write_register(REG_ACC, read_register(REG_ACC) < read_register(operand) ? 0 : 1);
			break;

		case OP_CMPLE:
			write_register(REG_ACC, read_register(operand) <= read_register(REG_ACC) ? 0 : 1);
			break;

		case OP_CMPGT:
			write_register(REG_ACC, read_register(operand) > read_register(REG_ACC) ? 0 : 1);
			break;

		case OP_CMPGE:
			write_register(REG_ACC, read_register(operand) >= read_register(REG_ACC) ? 0 : 1);
			break;

		case OP_CMPEQ:
			write_register(REG_ACC, read_register(operand) == read_register(REG_ACC) ? 0 : 1);
			break;

		case OP_CMPNE:
			write_register(REG_ACC, read_register(operand) != read_register(REG_ACC) ? 0 : 1);
			break;

		default:
			break;
		}
		write_register(REG_IP, next_ip);
	}
}

// read line by line and load into memory
void load_instructions(FILE * code_fp) {
	char line[128];
	while ( fgets ( line, sizeof(line), code_fp) != NULL ) {
         // parse line to generate instructions
		int opcode;
		int operand;
		sscanf(line, "%d %d", &opcode, &operand);
		instruction_t * instruction = &code_arr[code_arr_last];
		instruction->opcode = opcode;
		instruction->operand = operand;
		code_arr_last++;
		//printf("%s %d\n", opcode_to_text(opcode), operand);
	}

	write_register(REG_CP,0);
	write_register(REG_DP,0);
	write_register(REG_SP,0);
	write_register(REG_EP,0);
	write_register(REG_ACC,0);
	write_register(REG_R1,0);
	write_register(REG_IP,0);
	execute();


}


