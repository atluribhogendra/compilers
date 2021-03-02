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
       char * name;
       DataType type;
       LocationType loc_type;
       int memory_address;
       int register_label;
       int stack_offset;
} symtab_ele;

void init_string_tbl();
int insert_string(char * str);
void prt_string_tbl();
char * get_string(int index);

symtab_ele* find_symbol(char * name, DataType type);
symtab_ele* insert_symbol(char * name, DataType type);


#endif /* TABLE_H_ */
