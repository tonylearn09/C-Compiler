#ifndef __CODEGEN_H
#define __CODEGEN_H

#include "symbol_table.h"

int count_init(char *);
void push_global_buf(char *);
void gen_initialize();
void gen_var_decl(struct Entry *);
void gen_load_constant(type_type, struct Value *, char *);
void gen_load_value_val(struct Value *);
void gen_load_entry_val(struct Entry *);
void gen_rel_expr(struct Value *, struct Value *, Operator_Type);
void gen_arith_expr(struct Value *, struct Value *, Operator_Type op);
void gen_not_expr(struct Value *);
void gen_neg_expr(struct Value *);
void gen_logical_expr(struct Value *, struct Value *, Operator_Type op);
type_type stack_element_coercion(type_type v1_type, type_type v2_type);
void gen_assignment(struct Entry *entry, struct Value *val);
void gen_print_statement_start();
void gen_print_statement(struct Value *val);
void gen_read_statement(struct Entry *entry);
void gen_if_start();
void gen_else_statement();
void gen_if_exit();
void gen_for_start();
void gen_for_condition_jump();
void gen_for_body();
void gen_for_exit_incr();
void gen_while_start();
void gen_while_condition_jump();
void gen_while_exit();
void gen_do_while_start();
void gen_do_while_condition();
void gen_do_while_exit();
void gen_break_statement();
void gen_continue_statement();
void gen_function(struct Entry *entry);
void gen_end_function(struct Entry *entry);
void gen_function_call(struct Entry *entry);
void gen_return(struct Entry *entry, struct Value *val);
void gen_argu(struct Argu *argu, struct Value *val);
//void clear_global_buf();
void write_global_buf();
void push_global_buf(char *);

#endif
