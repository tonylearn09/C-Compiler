#ifndef __SYMCHK_H
#define __SYMCHK_H

#include "symbol_table.h"
#include "helper.h"

void push_symbol_table(struct SymbolTable *, struct SymbolTable *);
void check_remain_func(struct SymbolTable *, struct ErrorTable *);
struct Value *call_function(struct SymbolTable*, struct ErrorTable *, 
        const char *, struct Argu *);
bool check_in_loop_statement(bool in_loop, char *key, struct ErrorTable *err_table);
bool print_error(struct Value *val, struct ErrorTable *err_table);
bool read_error(struct Entry *entry, struct ErrorTable *err_table);
void assign_error(struct Entry *, struct Value *, struct ErrorTable *);

// Declaration check
int val_decl_check(struct SymbolTable*, struct Entry *, struct ErrorTable *);
int const_decl_check(struct SymbolTable*, struct Entry *, struct ErrorTable *);
int func_decl_check(struct SymbolTable*, struct Entry *, struct ErrorTable *);
int func_def_check(struct SymbolTable*, struct Entry *, struct ErrorTable *);
// Function return check
void return_type_check(struct ErrorTable *, struct Entry *, struct Value*);
void return_statement_check(struct ErrorTable *, struct Entry *, int *);

void bool_exp_check(struct ErrorTable *, struct Value *, const char *);

// compare type
int cmp_type(struct Type *, struct Type *);
int cmp_type_call(struct Type *, struct Type *);
int cmp_argu(struct Argu *, struct Argu *);
int cmp_argu_call(struct Argu *, struct Argu *);

struct Value *expr_neg(struct Value *, struct ErrorTable *);
struct Value *expr_not(struct Value *, struct ErrorTable *);
struct Value *expr_add(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_sub(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_mul(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_div(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_mod(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_and(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_or(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_lt(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_gt(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_le(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_ge(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_eq(struct Value *, struct Value *, struct ErrorTable *);
struct Value *expr_ne(struct Value *, struct Value *, struct ErrorTable *);

#endif
