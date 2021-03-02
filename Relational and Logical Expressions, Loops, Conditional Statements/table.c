#include "table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define STRTBL_LEN 3001       /* String table length */
#define STR_SPRTR  0          /* String seperator in string table */
#define SYMBOL_TBL_LEN 200    /* Symbol table size */

char     strg_tbl[STRTBL_LEN];   /* String table of length STRTBL_LEN */
int      last = 0;               /* end of the string table, empty */

void init_string_tbl()
{
   int i;
   for ( i=0; i<STRTBL_LEN; i++ )
      strg_tbl[i] = 0;
}

int insert_string(char * str) {
	printf("%s\n", str);
	int index = last;
	strcpy(strg_tbl + last, str);
	last += strlen(str) + 1;
	return index;
}

char * get_string(int index) {
	return strg_tbl + index;
}

void prt_string_tbl()
{
   int i;
   printf("String Table-------------------------------\n");
   for( i=0; i<last; i++ )
   {
     if( strg_tbl[i] == 0 )
       printf(" ");
     else
      printf("%c", strg_tbl[i]);
   }
   printf("\n");
}




symtab_ele symbol_tbl[SYMBOL_TBL_LEN];
int      symbol_tbl_last = 0;

symtab_ele* insert_symbol(char * name, DataType type) {
	symtab_ele* element = &symbol_tbl[symbol_tbl_last];
	element->type = type;
	element->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(element->name, name);
	element->memory_address = symbol_tbl_last;
	symbol_tbl_last++;
	return element;
}

symtab_ele* find_symbol(char * name, DataType type) {
	for (int i=0; i < symbol_tbl_last; i++) {
		symtab_ele* element = &symbol_tbl[i];
		if (type == element->type
			&& strcmp(name, element->name) == 0) {
			return element;
		}
	}
	return NULL;
}
