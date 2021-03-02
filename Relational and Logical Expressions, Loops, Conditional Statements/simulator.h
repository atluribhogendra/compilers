/*
 * simulator.h
 *
 *
 */

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#define ERROR_FILE_NOT_FOUND 2
#define ERROR_INVALID_TOKEN 3

typedef enum {
  OP_PUSH, OP_PUSH_IMMEDIATE,OP_LOAD, OP_STORE, OP_CLEARMEM, OP_POP,OP_ADD, OP_SUB,OP_MUL, OP_DIV, OP_MOD, OP_CALL, OP_JZ, OP_JMP, OP_JNZ, OP_HALT,
  OP_CMPLT,OP_CMPLE,OP_CMPEQ,OP_CMPNE,OP_CMPGT,OP_CMPGE
} opcode_s;

typedef enum {
 REG_ACC, REG_R1, REG_CP, REG_DP, REG_EP, REG_SP, REG_IP
} register_s;


typedef enum {
  F_NONE, F_PRINT_INT, F_PRINT_STR, F_PRINT_BOOL, F_READ_INT
} function_s;

typedef struct {
	opcode_s opcode;
	int operand;

} instruction_t;

void load_instructions(FILE * source_fp);

char* opcode_to_text(opcode_s opcode);

#endif /* SIMULATOR_H_ */
