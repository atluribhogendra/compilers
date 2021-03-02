/*
 * table.h
 *
 *  Created on: Apr 27, 2019
 *      Author: chand
 */

#ifndef TABLE_H_
#define TABLE_H_

typedef enum {
    SYMBOL_NULL, SYMBOL_VAR //,SYMBOL_FUNCTION
} SymbolType;

typedef enum {
	LOC_MEMORY, LOC_REGISTER, LOC_STACK
} LocationType;

typedef enum {
	DataType_NULL, DataType_INT4, DataType_String, DataType_Boolean
} DataType;

typedef struct {
       int id;
       int len;
       DataType type;
       LocationType loc_type;
       int memory_address;
       int register_label;
       int stack_offset;
} symtab_ele;


#endif /* TABLE_H_ */
