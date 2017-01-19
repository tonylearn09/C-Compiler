%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symbol_table.h"
#include "symchk.h"
#include "codegen.h"


extern int linenum;
extern FILE	*yyin;
extern char	*yytext;
extern char buf[256];
extern int Opt_Symbol; // in lex.l
extern int Opt_Statistic; // in lex.l

extern const char *TYPES[];
extern const char *KINDS[];

extern struct SymbolTable *sym_table;
extern struct SymbolTable *sym_table_buf;
extern struct ErrorTable *error_msg;
extern struct Entry *entry_buf;
extern struct ArrayNode *array_buf;
extern struct Entry *cur_func;
extern struct Argu *argu_buf;
extern int return_s;
extern int in_loop;
struct Type *type_buf;

int count;
int Opt_p = 0;
#define PPROCESS(t) {if(Opt_p) printf("<%s>\n",#t);}
%}

%union {
    int int_val;
    char *lexeme;
    struct Type *type;
    struct Value *value;
    struct Argu *argu;
    struct ValueArray *v_array;
    struct Entry *entry;
}

%type <int_val> array
%type <lexeme> identifier
%type <type> argu_type float_type bool_type
%type <value> value bool_value const_value
%type <value> expr var_init_val
%type <value> function_invoke initial_expr control_expr
%type <argu> funct_argu 
%type <v_array> var_array_init more_expr
%type <entry> funct_decl

%token	<lexeme> ID
%token	<lexeme> INT_CONST
%token	<lexeme> FLOAT_CONST
%token	<lexeme> SCIENTIFIC
%token	<lexeme> STR_CONST

%token	LE_OP
%token	NE_OP
%token	GE_OP
%token	EQ_OP
%token	AND_OP
%token	OR_OP

%token	<lexeme> READ
%token	<lexeme> BOOLEAN
%token	<lexeme> WHILE
%token	DO
%token	<lexeme> IF
%token	ELSE
%token	<lexeme> TRUE
%token	<lexeme> FALSE
%token	<lexeme> FOR
%token	<lexeme> INT
%token	<lexeme> PRINT
%token	<lexeme> BOOL
%token	<lexeme> VOID
%token	<lexeme> FLOAT
%token	<lexeme> DOUBLE
%token	<lexeme> STRING
%token	<lexeme> CONTINUE
%token	<lexeme> BREAK
%token	<lexeme> RETURN
%token  CONST

%token	L_PAREN
%token	R_PAREN
%token	COMMA
%token	SEMICOLON
%token	ML_BRACE
%token	MR_BRACE
%token	L_BRACE
%token	R_BRACE
%token	ADD_OP
%token	SUB_OP
%token	MUL_OP
%token	DIV_OP
%token	MOD_OP
%token	ASSIGN_OP
%token	LT_OP
%token	GT_OP
%token	NOT_OP

%left OR_OP AND_OP
%left NOT_OP
%left LT_OP GT_OP EQ_OP LE_OP GE_OP NE_OP
%left ADD_OP SUB_OP
%left MUL_OP DIV_OP MOD_OP
%left UMINUS

%start program
%%
Initialize: { gen_initialize(); }
          ;
program : Initialize decl_only_list decl_and_def_list
        ;
decl_only_list : declaration_list decl_only_list
               | func_definition_list
               ;

decl_and_def_list : decl_and_def_list declaration_list
                  | decl_and_def_list func_definition_list
                  |
                  ;
declaration_list : type var_decl SEMICOLON {
                    push_symbol_table(sym_table, sym_table_buf);
                    del_type(type_buf);
                    PPROCESS(reduce to declaration list);
                 }
                 | type funct_decl SEMICOLON {
                    if (func_decl_check(sym_table, $2, error_msg) == 0) {
                        insert_entry(sym_table, $2);
                        //insert_argu(sym_table, $2->attr->argu);
                    }
                    else
                        del_entry($2);

                    del_type(type_buf);
                    PPROCESS(reduce to declaration list);
                 }
                 | CONST type const_decl SEMICOLON {
                    push_symbol_table(sym_table, sym_table_buf);
                    del_type(type_buf);
                    PPROCESS(reduce to declaration list);
                 }
                 ;
func_definition_list : type funct_decl {
                        int return_val = func_def_check(sym_table, $2,
                            error_msg);
                        del_type(type_buf); // Don't need
                        if (return_val == 0) {
                            // First time to see this function 
                            insert_entry(sym_table, $2); 
                            (sym_table->cur_level)++;
                            (sym_table_buf->cur_level)++;
                            gen_function($2);
                            insert_argu(sym_table, $2->attr->argu,
                                sym_table->cur_level, error_msg);
                            cur_func = $2;
                            //cur_func->decl = 1;
                            $2->decl = true;
                        } else if (return_val == 1) {
                            // No need to push id to symbol_table
                            // Already in it (have declarion at front
                            struct Entry *finded_entry =
                                look_up_entry(sym_table, $2->name);
                            (sym_table->cur_level)++;
                            (sym_table_buf->cur_level)++;
                            gen_function($2);
                            insert_argu(sym_table, $2->attr->argu,
                                sym_table->cur_level, error_msg);
                            cur_func = finded_entry;
                            //cur_func->decl = 1;
                            finded_entry->decl = true;
                        } else {
                            // Error occur
                            (sym_table->cur_level)++;
                            (sym_table_buf->cur_level)++;
                            cur_func = $2;
                            gen_function($2);
                        }
                     } L_BRACE statement {
                        return_statement_check(error_msg, cur_func, &return_s);
                     } R_BRACE {
                        cur_func = NULL;
                        PPROCESS(reduce to function_def);
                     } leave_new_scope  {
                        gen_end_function($2);
                     }
                     ;
var_decl : var_decl_kinds {
            insert_entry(sym_table_buf, copy_entry(entry_buf));
            PPROCESS(I am inserted);
            clear_entry(entry_buf);
         }
         | var_decl COMMA var_decl_kinds {
            insert_entry(sym_table_buf, copy_entry(entry_buf));
            clear_entry(entry_buf);
         }
         ;

var_decl_kinds : identifier {
                    assign_entry(entry_buf, $1, VARIABLE_t,
                        sym_table->cur_level, copy_type(type_buf), NULL);
                    gen_var_decl(entry_buf);
                }
                | identifier decl_array {
                    assign_entry(entry_buf, $1, VARIABLE_t, 
                        sym_table->cur_level, copy_type(type_buf), NULL);
                    entry_buf->type->arr = copy_array(array_buf);
                    del_array(&array_buf);
                }
                | identifier decl_array ASSIGN_OP var_array_init {
                    assign_entry(entry_buf, $1, VARIABLE_t, 
                        sym_table->cur_level, copy_type(type_buf), NULL);
                    entry_buf->type->arr = copy_array(array_buf);
                    del_array(&array_buf);
                    assign_valuearray(entry_buf, $4);
                }
                | identifier ASSIGN_OP var_init_val {
                    assign_entry(entry_buf, $1, VARIABLE_t,
                        sym_table->cur_level, copy_type(type_buf), NULL);
                    init_value(entry_buf, $3);
                    gen_var_decl(entry_buf);
                    gen_assignment(entry_buf, $3);
                }
                ;

var_init_val : expr {$$ = $1; }
         ;
var_array_init : L_BRACE more_expr R_BRACE {$$ = $2; }
               | L_BRACE R_BRACE {$$ = NULL; }
               ;

more_expr : expr {
            $$ = build_valuearray();
            insert_valuearray($$, $1);
          }
          | more_expr COMMA expr {
            insert_valuearray($1, $3);
          }
          ;

funct_decl : identifier L_PAREN arguments R_PAREN {
            count = count_init($1);
            assign_entry(entry_buf, $1, FUNCTION_t, sym_table->cur_level,
                copy_type(type_buf), build_attribute(copy_argu(argu_buf), NULL));
            del_argu(&argu_buf);
            $$ = copy_entry(entry_buf);
            clear_entry(entry_buf);
            //printf("ID: %s\n", $1);
            //printf("count: %d\n", count);
            PPROCESS(reduce to funct_decl)
           }
           | identifier L_PAREN R_PAREN {
            count = count_init($1);
            assign_entry(entry_buf, $1, FUNCTION_t, sym_table->cur_level,
                copy_type(type_buf), build_attribute(NULL, NULL));
            $$ = copy_entry(entry_buf);
            clear_entry(entry_buf);
           }
           ;
arguments : arguments COMMA argu_type identifier decl_array {
            $3->arr = copy_array(array_buf);
            del_array(&array_buf);
            add_argu(&argu_buf, $4, $3);
          }
          | arguments COMMA argu_type identifier {
            add_argu(&argu_buf, $4, $3);
          }
          | argu_type identifier decl_array {
            $1->arr = copy_array(array_buf);
            del_array(&array_buf);
            add_argu(&argu_buf, $2, $1);
          }
          | argu_type identifier {
          //printf("%s: %s\n", $2, $1->type);
            add_argu(&argu_buf, $2, $1);
          }
          ;

const_decl : const_decl COMMA identifier ASSIGN_OP const_value {
            assign_entry(entry_buf, $3, CONSTANT_t, sym_table->cur_level,
                copy_type(type_buf), NULL);
            init_value(entry_buf, $5);
            insert_entry(sym_table_buf, copy_entry(entry_buf));
            clear_entry(entry_buf);
           }
           | identifier ASSIGN_OP const_value {
            assign_entry(entry_buf, $1, CONSTANT_t, sym_table->cur_level,
                copy_type(type_buf), NULL);
            init_value(entry_buf, $3);
            insert_entry(sym_table_buf, copy_entry(entry_buf));
            clear_entry(entry_buf);
           }
           ;

decl_array : decl_array ML_BRACE INT_CONST MR_BRACE {
            add_dim(&array_buf, atoi($3));
           }
           | ML_BRACE INT_CONST MR_BRACE {
            add_dim(&array_buf, atoi($2));
            PPROCESS(reduce to decl_array)
           }
           ;

go_new_scope : { 
                (sym_table->cur_level)++;
                (sym_table_buf->cur_level)++;
            }
            ;
leave_new_scope : {
                if (Opt_Symbol)
                    print_symbol_table(sym_table);
                pop_symbol_table(sym_table);
                pop_symbol_table(sym_table_buf);
                }
                ;

statement : statement type var_decl SEMICOLON {
            //print_symbol_table(sym_table_buf);
            return_s = 0;
            push_symbol_table(sym_table, sym_table_buf);
            del_type(type_buf);
            //print_symbol_table(sym_table);
          }
          | statement CONST type const_decl SEMICOLON {
            return_s = 0;
            push_symbol_table(sym_table, sym_table_buf);
            del_type(type_buf);
          }
          | statement assignment SEMICOLON {return_s = 0; }
          | statement PRINT {
            gen_print_statement_start();
          } expr SEMICOLON {
            if (!print_error($4, error_msg)) {
                gen_print_statement($4);
            }
            return_s = 0;
          }
          | statement READ identifier SEMICOLON {
            struct Entry *finded_entry = find_id(sym_table, error_msg, $3);
            if (!read_error(finded_entry, error_msg)) {
                if (finded_entry) {
                    gen_read_statement(finded_entry);
                }
            }
            return_s = 0;
          }
          | statement IF L_PAREN expr {
            bool_exp_check(error_msg, $4, $2);
            gen_if_start();
          } R_PAREN L_BRACE go_new_scope statement R_BRACE leave_new_scope {
            gen_else_statement();
          } else_statement {
            gen_if_exit();
            return_s = 0;
          }
          | statement WHILE L_PAREN {
            gen_while_start();
          } expr {
            bool_exp_check(error_msg, $5, $2);
            gen_while_condition_jump();
          } R_PAREN L_BRACE go_new_scope {
            in_loop++;
          } statement R_BRACE leave_new_scope {
            gen_while_exit();
            return_s = 0;
            in_loop--;
          }
          | statement DO L_BRACE go_new_scope {
            gen_do_while_start();
            in_loop++;
          } statement R_BRACE WHILE L_PAREN {
            gen_do_while_condition();
          } expr {
            bool_exp_check(error_msg, $11, $8);
          } R_PAREN SEMICOLON leave_new_scope {
            gen_do_while_exit();
            return_s = 0;
            in_loop--;
          }
          | statement FOR L_PAREN initial_expr SEMICOLON {
            gen_for_start();
          } control_expr {
            bool_exp_check(error_msg, $7, $2);
            gen_for_condition_jump();
          } SEMICOLON initial_expr R_PAREN L_BRACE go_new_scope {
            gen_for_body();
            in_loop++;
          } statement R_BRACE leave_new_scope {
            gen_for_exit_incr();
            return_s = 0;
            in_loop--;
          }
          | statement RETURN expr SEMICOLON {
            return_s = 1;
            return_type_check(error_msg, cur_func, $3);
            gen_return(cur_func, $3);
          }
          | statement BREAK SEMICOLON {
            return_s = 0;
            if (check_in_loop_statement(in_loop, $2, error_msg)) {
                gen_break_statement();
            }
          }
          | statement CONTINUE SEMICOLON {
            return_s = 0;
            if (check_in_loop_statement(in_loop, $2, error_msg)) {
                gen_continue_statement();
            }
          }
          | statement SEMICOLON {return_s = 0; }
          | statement L_BRACE go_new_scope {
          } statement R_BRACE leave_new_scope {
            return_s = 0;
          }
          | statement function_invoke SEMICOLON {
            return_s = 0;
          }
          |
          ;

else_statement : ELSE L_BRACE go_new_scope
                statement R_BRACE leave_new_scope 
               |
              ;

initial_expr : expr {$$ = $1; }
             | assignment {$$ = NULL; }
             | {$$ = NULL; }
             ;

control_expr : expr {$$ = $1;}
             | {$$ = NULL; }
             ;

expr : L_PAREN expr R_PAREN {$$ = $2; }
     | SUB_OP expr %prec UMINUS  {
        gen_neg_expr($2);
        $$ = expr_neg($2, error_msg);
        PPROCESS(reduce to expr from sum_op)
     }
    | NOT_OP expr {
        gen_not_expr($2);
        $$ = expr_not($2, error_msg);
    }
    | expr ADD_OP expr {
        gen_arith_expr($1, $3, ADD);
        $$ = expr_add($1, $3, error_msg);
    }
    | expr SUB_OP expr {
        gen_arith_expr($1, $3, SUB);
        $$ = expr_sub($1, $3, error_msg);
    }
    | expr MUL_OP expr {
        gen_arith_expr($1, $3, MUL);
        $$ = expr_mul($1, $3, error_msg);
    }
    | expr DIV_OP expr {
        gen_arith_expr($1, $3, DIV);
        $$ = expr_div($1, $3, error_msg);
    }
    | expr MOD_OP expr {
        gen_arith_expr($1, $3, MOD);
        $$ = expr_mod($1, $3, error_msg);
    }
    | expr AND_OP expr {
        gen_logical_expr($1, $3, AND);
        $$ = expr_and($1, $3, error_msg);
    }
    | expr OR_OP expr {
        gen_logical_expr($1, $3, OR);
        $$ = expr_or($1, $3, error_msg);
    }
    | expr LT_OP expr {
        gen_rel_expr($1, $3, LESS);
        $$ = expr_lt($1, $3, error_msg);
    }
    | expr GT_OP expr {
        gen_rel_expr($1, $3, GREAT);
        $$ = expr_gt($1, $3, error_msg);
    }
    | expr LE_OP expr {
        gen_rel_expr($1, $3, LESS_EQ);
        $$ = expr_le($1, $3, error_msg);
        PPROCESS(reduce to expr from le_op)
    }
    | expr GE_OP expr {
        gen_rel_expr($1, $3, GREAT_EQ);
        $$ = expr_ge($1, $3, error_msg);
    }
    | expr EQ_OP expr {
        gen_rel_expr($1, $3, EQUAL);
        $$ = expr_eq($1, $3, error_msg);
    }
    | expr NE_OP expr {
        gen_rel_expr($1, $3, NEQUAL);
        $$ = expr_ne($1, $3, error_msg);
    }
    | value {
        $$ = $1;
        gen_load_value_val($$);
    }
    | identifier {
        struct Entry *entry = find_id(sym_table, error_msg, $1);
        //printf("name: %s, type: %s\n", entry->name, entry->type->type);
        if (entry) {
            if (entry->kind != CONSTANT_t) {
                $$ = build_default_value(copy_type(entry->type));
            } else {
                $$ = entry->attr->value;
            }
            gen_load_entry_val(entry);
        } else {
            $$ = NULL;
        }
    }
    | identifier array {
        struct Entry *entry = copy_entry(find_id(sym_table, error_msg, $1));
        if (entry) 
            reduce_type_array(entry->type, $2, error_msg);
        if (entry) {
            $$ = build_default_value(copy_type(entry->type));
        } else {
            $$ = NULL;
        }
    }
    | function_invoke {$$ = $1; }
    ;

assignment : identifier array ASSIGN_OP expr {
            struct Entry *entry = copy_entry(find_id(sym_table, error_msg, $1));
            //printf("name: %s type: %d", entry->name);
            //printf("times: %d\n", $2);
            if (entry)
                reduce_type_array(entry->type, $2, error_msg);
            assign_error(entry, $4, error_msg);
            del_entry(entry);
            PPROCESS(reduce to assignment);
           }
           | identifier ASSIGN_OP expr {
            PPROCESS(reduce to assignment);
            struct Entry *temp = find_id(sym_table, error_msg, $1);
            assign_error(temp, $3, error_msg);
            gen_assignment(temp, $3);
           }
           ;

function_invoke : identifier L_PAREN {
                    struct Entry *finded_entry = find_id(sym_table, error_msg, $1);
                    if (finded_entry && finded_entry->attr)
                        argu_buf = finded_entry->attr->argu;
                } funct_argu R_PAREN {
                    struct Entry *finded_entry = find_id(sym_table, error_msg, $1);
                    $$ = call_function(sym_table, error_msg, $1, $4);
                    gen_function_call(finded_entry);
                    PPROCESS(reduce to function_invoke);
                }
                ;

funct_argu : expr {
            if ($1 && argu_buf) {
                $$ = build_argu($1->type);
                gen_argu(argu_buf, $1);
                argu_buf = argu_buf->next;
            }
            PPROCESS(reduce to funct_argu);
            //printf("%s", $1->type->type);
           }
           | funct_argu COMMA expr {
            if ($3 && argu_buf) {
                $$->next = build_argu($3->type);
                gen_argu(argu_buf, $3);
                argu_buf = argu_buf->next;
            }
           }
           | {$$ = NULL; }
          ;
array : ML_BRACE expr MR_BRACE {
        $$ = 1;
        if ($2 != NULL &&
        ($2->type->arr != NULL || $2->type->type != INT_t)) {
            char msg[1024];
            strncpy(msg, "The index of array must be integer", sizeof(msg));
            push_error_table(error_msg, msg);
        } else if ($2 == NULL) {
            char msg[1024];
            strncpy(msg, "The index of array must be integer", sizeof(msg));
            push_error_table(error_msg, msg);
        }
      }
      | array ML_BRACE expr MR_BRACE {
        $$ += 1;
        if ($3 != NULL &&
        ($3->type->arr != NULL || $3->type->type != INT_t)) {
            char msg[1024];
            strncpy(msg, "The index of array must be integer", sizeof(msg));
            push_error_table(error_msg, msg);
        } else if ($3 == NULL) {
            char msg[1024];
            strncpy(msg, "The index of array must be integer", sizeof(msg));
            push_error_table(error_msg, msg);
        }
      }
     ;

type : INT {type_buf = build_type(INT_t, NULL); }
     | float_type  {type_buf = $1; }
     | STRING {type_buf = build_type(STRING_t, NULL); }
     | bool_type {type_buf = $1; }
     | VOID {type_buf = build_type(VOID_t, NULL); }
     ;

argu_type : INT {$$ = build_type(INT_t, NULL); }
          | float_type {$$ = $1; }
         | STRING {$$ = build_type(STRING_t, NULL); }
         | bool_type {$$ = $1; }
         ;

float_type : FLOAT {$$ = build_type(FLOAT_t, NULL); }
           | DOUBLE {$$ = build_type(DOUBLE_t, NULL); }
          ;
bool_type : BOOLEAN {$$ = build_type(BOOLEAN_t, NULL); }
          | BOOL {$$ = build_type(BOOL_t, NULL); }
         ;

identifier : ID {$$ = strdup($1); }
           ;
value : bool_value {$$ = $1; }
      | INT_CONST {$$ = build_value(build_type(INT_t, NULL), $1); }
      | FLOAT_CONST {$$ = build_value(build_type(FLOAT_t, NULL), $1); }
      | STR_CONST {$$ = build_value(build_type(STRING_t, NULL), $1); }
      | SCIENTIFIC {$$ = build_value(build_type(FLOAT_t, NULL), $1); }
      ;
const_value : bool_value {$$ = $1;}
            | INT_CONST {$$ = build_value(build_type(INT_t, NULL), $1); }
           | SUB_OP INT_CONST {
            char neg_val[20];
            strcpy(neg_val, "-");
            strcat(neg_val, $2);
            //sprintf(neg_val, "-%s", $2);
            $$ = build_value(build_type(INT_t, NULL), neg_val);
           }
           | FLOAT_CONST {
            $$ = build_value(build_type(FLOAT_t, NULL), $1);
           }
           | SUB_OP FLOAT_CONST {
            char neg_val[20];
            strcpy(neg_val, "-");
            strcat(neg_val, $2);
            //sprintf(neg_val, "-%s", $2);
            $$ = build_value(build_type(FLOAT_t, NULL), neg_val);
           }
           | STR_CONST {
            $$ = build_value(build_type(STRING_t, NULL), $1); 
           }
           | SCIENTIFIC {
            $$ = build_value(build_type(FLOAT_t, NULL), $1); 
           }
           | SUB_OP SCIENTIFIC {
            char neg_val[20];
            strcpy(neg_val, "-");
            strcat(neg_val, $2);
            //sprintf(neg_val, "-%s", $2);
            $$ = build_value(build_type(FLOAT_t, NULL), neg_val);
           }
           ;

bool_value : TRUE {
            $$ = build_value(build_type(BOOL_t, NULL), "true");
           }
           | FALSE {
            $$ = build_value(build_type(BOOL_t, NULL), "false");
           }
           ;

%%

int yyerror( char *msg )
{
    fprintf( stderr, "\n|--------------------------------------------------------------------------\n" );
	fprintf( stderr, "| Error found in Line #%d: %s\n", linenum, buf );
	fprintf( stderr, "|\n" );
	fprintf( stderr, "| Unmatched token: %s\n", yytext );
	fprintf( stderr, "|--------------------------------------------------------------------------\n" );
	exit(-1);
	//  fprintf( stderr, "%s\t%d\t%s\t%s\n", "Error found in Line ", linenum, "next token: ", yytext );
}

