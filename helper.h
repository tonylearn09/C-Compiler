#ifndef __HELPER_H
#define __HELPER_H

#include "symbol_table.h"

bool is_float_coercion_valid(struct Type *);
bool is_double_coercion_valid(struct Type *);
bool double_coercion_decl(struct Value *);
bool float_coercion_decl(struct Value *);
void decl_defaul_value(struct Entry *, struct Value **);
void print_invlaid_initial_err(char *, struct Attribute **, struct ErrorTable *);
void print_invlaid_initial_corecion_err(char *, struct Attribute **, struct Entry *, struct ErrorTable *);
void print_invalid_initial_array_coercion_err(char *msg, struct Value **value, struct Entry *entry, struct ErrorTable *err_table);
int get_array_size(struct ArrayNode *cur, char *msg, struct ErrorTable *err_table);
bool expr_check_invlaid_type_helper(struct Value **lhs, struct Value **rhs, const char *operator, struct ErrorTable *err_table);
bool expr_check_invlaid_type_logic(struct Value **lhs, struct Value **rhs, const char *operator, struct ErrorTable *err_table);
bool expr_check_invlaid_type(struct Value **lhs, struct Value **rhs, const char *operator, struct ErrorTable *err_table);
void expr_evaluate(struct Value *lhs, struct Value *rhs, const char *operator);
void logic_expr_evaluate(struct Value *lhs, struct Value *rhs, char *operator);
#endif
