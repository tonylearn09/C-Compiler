#include <stdio.h>
#include <stdlib.h>
#include "symbol_table.h"
#include "symchk.h"

extern int yyparse();
extern FILE* yyin;
extern int Opt_Symbol;
extern int Opt_Statistic;
struct SymbolTable *sym_table;
struct SymbolTable *sym_table_buf;
struct ErrorTable *error_msg;
struct Entry *entry_buf;
struct ArrayNode *array_buf;
struct Entry *cur_func;
struct Argu *argu_buf;
int return_s;
int in_loop;
int expr_label;
struct LabelStack *label_stack;

FILE *outfile;
int  main( int argc, char **argv )
{

	if( argc == 1 )
	{
		yyin = stdin;
	}
	else if( argc == 2 )
	{
		FILE *fp = fopen( argv[1], "r" );
		if( fp == NULL ) {
				fprintf( stderr, "Open file error\n" );
				exit(-1);
		}
		yyin = fp;
	}
	else
	{
	  	fprintf( stderr, "Usage: ./parser [filename]\n" );
   		exit(0);
 	} 

    // Initialize
    outfile = fopen("output.j", "w");
    if (!outfile) {
        printf("Can't write to file error!\n");
        exit(1);
    }

    sym_table = build_symbol_table();
    sym_table_buf = build_symbol_table();
    error_msg = build_error_table();
    entry_buf = (struct Entry *) malloc(sizeof(struct Entry));
    array_buf = NULL;
    del_argu(&argu_buf);
    cur_func = NULL;
    return_s = 0;
    in_loop = 0;
    expr_label = 0;
    label_stack = build_label_stack();

    yyparse();	/* primary procedure of parser */

    check_remain_func(sym_table, error_msg);
    if (Opt_Symbol)
        print_symbol_table(sym_table);
    print_error_table(error_msg);
  	if(Opt_Statistic){
        print_statistic();
	}
    if (error_msg->size == 0) {
        //fprintf( stdout, "\n|---------------------------------------------|\n" );
        //fprintf( stdout, "|  There is no syntactic and samentic error!  |\n" );
        //fprintf( stdout, "|---------------------------------------------|\n" );
        del_symbol_table(sym_table);
        del_label_stack(label_stack);
        exit(0);
    }


    
    //fprintf( stdout, "\n|--------------------------------|\n" );
    //fprintf( stdout, "|  There is no syntactic error!  |\n" );
    //fprintf( stdout, "|--------------------------------|\n" );
    
    del_symbol_table(sym_table);
    del_error_table(error_msg);
    del_label_stack(label_stack);
    exit(0);
}

