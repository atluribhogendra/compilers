#define STRTBL_LEN 3001       /* String table length */
#define STR_SPRTR  0          /* String seperator in string table */
#define SYMBOL_TBL_LEN 200    /* Symbol table size */

#define SYMBOL_NOT_FOUND -1

char     strg_tbl[STRTBL_LEN];   /* String table of length STRTBL_LEN */
int      last = 0;               /* end of the string table, empty */

void init_string_tbl()
{
   int i;
   for ( i=0; i<STRTBL_LEN; i++ )
      strg_tbl[i] = 0;
}

void prt_string_tbl()
{
   int i;

   for( i=0; i<last; i++ )
   {
     if( strg_tbl[i] == 0 )
       printf(" ");
     else
      printf("%c", strg_tbl[i]);
   }
   printf("\n");
}


typedef enum {
    SYMBOL_NULL, SYMBOL_VAR //,SYMBOL_FUNCTION
} SymbolType;

typedef enum {
	LOC_MEMORY, LOC_REGISTER, LOC_STACK
} LocationType;

typedef enum {
	Typ_NULL, Typ_INT4, Yup_Literal
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


symtab_ele symbol_tbl[SYMBOL_TBL_LEN];
int      symbol_tbl_last = 0;


int insertSymbol(symtab_ele element) {
	int index = symbol_tbl_last;
	symbol_tbl[symbol_tbl_last++] = element;
	return index;
}

int findSymbol(char * name, DataType type) {
	for (int i=0; i < symbol_tbl_last; i++) {
		symtab_ele element = symbol_tbl[symbol_tbl_last];
		if (type == element.type
			&& strlen(name) == element.len
			&& strncmp(name, element.id, element.len)) {
			return i;
		}
	}
	return SYMBOL_NOT_FOUND;
}
