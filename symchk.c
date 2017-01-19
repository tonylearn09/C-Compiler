#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symchk.h"

extern int linenum; // lex
extern struct ErrorTable *error_msg; // parser
extern const char *TYPES[];
extern const char *KINDS[];


// Not sure this function
void push_symbol_table(struct SymbolTable *new_sym_table, struct SymbolTable *old_sym_table)
{
    if (new_sym_table && old_sym_table) {
        for (int i = 0; i < old_sym_table->size; ++i) {
            if ((old_sym_table->entries[i]->kind == VARIABLE_t ||
                        old_sym_table->entries[i]->kind == CONSTANT_t) &&
                    val_decl_check(new_sym_table, old_sym_table->entries[i], error_msg) != 1) {
                insert_entry(new_sym_table, old_sym_table->entries[i]);
            } else {
                del_entry(old_sym_table->entries[i]);
            }
        }
        old_sym_table->size = 0;
    }
}

void check_remain_func(struct SymbolTable *sym_table, struct ErrorTable *err_table)
{
    if (sym_table && err_table) {
        //int size = sym_table->size;
        char error_massege[1024];
        for (int i = 0; i < sym_table->size; ++i) {
            if (sym_table->entries[i]->kind == FUNCTION_t &&
                    sym_table->entries[i]->decl == false) {
                sprintf(error_massege, "Function '%s' has no definition yet",
                        sym_table->entries[i]->name);
                // Add this entry to type and kind Unknown, attr be NULL
                // so won't show up compile error again
                push_error_table(err_table, error_massege);
            }
        }
    }
}

struct Value *call_function(struct SymbolTable *sym_table, struct ErrorTable *err_table, const char *func_id, struct Argu *arg)
{
    struct Entry *finded_entry = find_id(sym_table, err_table, func_id);
    if (finded_entry) {
        char msg[1024];
        if (finded_entry->kind != FUNCTION_t) {
            // Add this entry to type and kind Unknown, attr be NULL
            // so won't show up compile error again
            /*insert_entry(sym_table, build_entry(func_id, "Unknown", sym_table->cur_level,
                    build_type("Unknown", NULL), build_attribute(arg, NULL)));*/
            sprintf(msg, "'%s' is not a function", func_id);
            push_error_table(err_table, msg);
            return NULL;
        }

        struct Argu *func_argu;
        if (finded_entry->attr == NULL) 
            func_argu = NULL;
        else
            func_argu = finded_entry->attr->argu;

        if (cmp_argu_call(func_argu, arg) != 0) {
            sprintf(msg, "Invalid type coercion in arguement");
            push_error_table(err_table, msg);
        }
        return build_default_value(copy_type(finded_entry->type));
    } else
        return NULL;
}

bool read_error(struct Entry *entry, struct ErrorTable *err_table)
{
    if (entry != NULL && entry->type->arr != NULL) {
        char msg[1024];
        strcpy(msg, "Variable references in 'read' statement must be scalar type");
        push_error_table(error_msg, msg);
        return true;
    } else {
        return false;
    }
}

bool print_error(struct Value *val, struct ErrorTable *err_table)
{
    if (val != NULL && val->type->arr != NULL) {
        char msg[1024];
        strcpy(msg, "Variable references in 'print' statement must be scalar type");
        push_error_table(error_msg, msg);
        return true;
    } else {
        return false;
    }
}

bool check_in_loop_statement(bool in_loop, char *key, struct ErrorTable *err_table)
{
    if (in_loop == 0) {
        char msg[1024];
        sprintf(msg, "'%s' can only appear in loop statement", key);
        push_error_table(err_table, msg);
        return false;
    } else {
        return true;
    }
}


void assign_error(struct Entry * entry, struct Value *val, struct ErrorTable *err_table)
{
    if (entry && val) {
        char msg[1024];
        if (entry->kind == FUNCTION_t) {
            sprintf(msg, "Invalid function: '%s' assignment", entry->name);
            push_error_table(err_table, msg);
        } else if (entry->kind == CONSTANT_t) {
            sprintf(msg, "Invalid constant: '%s' assignment", entry->name);
            push_error_table(err_table, msg);
        } else if (entry->type->arr != NULL || val->type->arr!= NULL) {
            sprintf(msg, "Invalid array assignment");
            push_error_table(err_table, msg);
        } else if (entry->type->type == DOUBLE_t) {
            if (!is_double_coercion_valid(val->type)) {
                sprintf(msg, "Invalid type coercion: %s -> double", TYPES[get_type_string(val->type->type)]);
                push_error_table(err_table, msg);
            }
        } else if (entry->type->type == FLOAT_t) {
            if (!is_float_coercion_valid(val->type)) {
                sprintf(msg, "Invalid type coercion: %s -> float", TYPES[get_type_string(val->type->type)]);
                push_error_table(err_table, msg);
            }
        } else if (entry->type->type != val->type->type) {
            sprintf(msg, "Invalid type coercion: %s -> %s", 
                    TYPES[get_type_string(val->type->type)], TYPES[get_type_string(entry->type->type)]);
            push_error_table(err_table, msg);
        }
    }
}


// Declaration check
int val_decl_check(struct SymbolTable *sym_table, struct Entry *entry, struct ErrorTable *err_table)
{
    int reval = const_decl_check(sym_table, entry, err_table);
    if (reval == -1) // NULL
        return reval;

    char msg[1024];
    if (entry->type->arr != NULL && entry->attr != NULL && entry->attr->v_array == NULL) {
        // Array, no intial value
        int array_size = get_array_size(entry->type->arr, msg, err_table);
        if (array_size == 0)
            return 1; 
    } else if (entry->type->arr != NULL && entry->attr != NULL && entry->attr->v_array != NULL) { 
        // Array, with initial value
        int array_size = get_array_size(entry->type->arr, msg, err_table);
        if (array_size == 0)
            return 1;
        if (entry->attr->v_array->size > array_size) {
            sprintf(msg, "Too many initialize values for array");
            push_error_table(err_table, msg);
            reval = 1;
            for (int i = array_size; i < entry->attr->v_array->size; ++i) {
                del_val(entry->attr->v_array->values[i]);
            }
            entry->attr->v_array->size = array_size;
        }
        for (int i = entry->attr->v_array->size; i < array_size; ++i) {
            insert_valuearray(entry->attr->v_array, build_default_value(copy_type(entry->type)));
        }

        for (int i = 0; i < array_size; ++i) { // Each index type coercion
            if (entry->type->type == DOUBLE_t) {
                if (entry->attr->v_array->values[i] == NULL)
                    //entry->attr->v_array->values[i] = build_value(build_type("double",NULL), "0.0");
                    entry->attr->v_array->values[i] = build_default_value(build_type(DOUBLE_t, NULL));
                else if (!double_coercion_decl(entry->attr->v_array->values[i])) {
                    print_invalid_initial_array_coercion_err(msg, &(entry->attr->v_array->values[i]), entry, err_table);
                    decl_defaul_value(entry, &entry->attr->v_array->values[i]);
                    reval = 1;
                    return reval;
                } 
            } else if (entry->type->type == FLOAT_t) {
                if (entry->attr->v_array->values[i] == NULL)
                    //entry->attr->v_array->values[i] = build_value(build_type("float",NULL), "0.0");
                    entry->attr->v_array->values[i] = build_default_value(build_type(FLOAT_t, NULL));
                else if (!float_coercion_decl(entry->attr->v_array->values[i])) {
                    print_invalid_initial_array_coercion_err(msg, &(entry->attr->v_array->values[i]), entry, err_table);
                    decl_defaul_value(entry, &entry->attr->v_array->values[i]);
                    reval = 1;
                    return reval;
                } 
            } else {
                if (entry->attr->v_array->values[i] == NULL) {
                    decl_defaul_value(entry, &entry->attr->v_array->values[i]);
                } else if (entry->attr->v_array->values[i]->type->type != entry->type->type) {
                    print_invalid_initial_array_coercion_err(msg, &(entry->attr->v_array->values[i]), entry, err_table);
                    decl_defaul_value(entry, &entry->attr->v_array->values[i]);
                    reval = 1;
                    return reval;
                }
                    
            }

        }
    }
    return reval;
}

int const_decl_check(struct SymbolTable *sym_table, struct Entry *entry, struct ErrorTable *err_table)
{
    if (sym_table == NULL || entry == NULL || err_table == NULL)
        return -1;

    char msg[1024];
    int reval = 0;
    struct Entry *finded_entry;
    if ((finded_entry = look_up_entry(sym_table, entry->name)) != NULL) {
        if (finded_entry->level == entry->level) {
            sprintf(msg, "Identifier '%s' has been declared in this scope", entry->name);
            push_error_table(err_table, msg);
            reval = 1;
        }
    }

    if (entry->type->type == VOID_t) {
        strncpy(msg, "Value type should not be 'void'", sizeof(msg));
        push_error_table(err_table, msg);
        reval = 1;
    }

    if (entry->type->arr == NULL && entry->attr != NULL && entry->attr->value != NULL) {
        if (entry->type->type == DOUBLE_t) { 
            if (!double_coercion_decl(entry->attr->value)) {
                print_invlaid_initial_corecion_err(msg, &(entry->attr), entry, err_table);
                reval = 1;
            }
        } else if (entry->type->type == FLOAT_t) {
            if (!float_coercion_decl(entry->attr->value)) {
                print_invlaid_initial_corecion_err(msg, &(entry->attr), entry, err_table);
                reval = 1;
            }
        } else if (entry->attr->value->type->type != entry->type->type) {
            print_invlaid_initial_corecion_err(msg, &(entry->attr), entry, err_table);
            reval = 1;
        }
    }
    return reval;
}

int func_decl_check(struct SymbolTable *sym_table, struct Entry *entry, struct ErrorTable *err_table)
{
    if (sym_table == NULL || entry == NULL || err_table == NULL)
        return -1;
    char msg[1024];
    if (look_up_entry(sym_table, entry->name)) {
        sprintf(msg, "Identifier '%s' has been declard", entry->name);
        push_error_table(err_table, msg);
        return 1;
    }
    return 0;
}

int func_def_check(struct SymbolTable *sym_table, struct Entry *entry, struct ErrorTable *err_table)
{
    if (sym_table == NULL || entry == NULL || err_table == NULL)
        return -1;
    char msg[1024];
    struct Entry *finded_entry;
    int reval = 0;
    if (finded_entry = look_up_entry(sym_table, entry->name)) {
        reval = 1; /* 1: No need to push this function to symboltable */
        if (cmp_type(entry->type, finded_entry->type) != 0 ||
                cmp_argu(entry->attr->argu, finded_entry->attr->argu) != 0) {
            strncpy(msg, "Function definition different from declaration", sizeof(msg));
            push_error_table(err_table, msg);
            reval = 2;
        }
        if (finded_entry->decl == true) {
            sprintf(msg, "Redefinition of function '%s'", finded_entry->name);
            push_error_table(err_table, msg);
            reval = 2;
        }
    }
    return reval;
}
// Function return check
void return_statement_check(struct ErrorTable *err_table, struct Entry *entry, int *return_s)
{
    if (err_table && entry) {
        if (*return_s != 1 && entry->type->type != VOID_t) {
            char msg[1024];
            strncpy(msg, "The last statement must be a return statement", sizeof(msg));
            push_error_table(err_table, msg);
        }
        *return_s = 0;
    }
}
void return_type_check(struct ErrorTable *err_table, struct Entry *entry, struct Value *value)
{
    if (err_table && entry && value) {
        char msg[1024];
        if (value->type->arr != NULL) {
            strncpy(msg, "Return type can't be array", sizeof(msg));
            push_error_table(err_table, msg);
            return;
        }

        if (entry->type->type == DOUBLE_t) { 
            if (!is_double_coercion_valid(value->type)) {
                sprintf(msg, "Return type coercion error: %s -> %s",
                        TYPES[get_type_string(value->type->type)], TYPES[get_type_string(entry->type->type)]);
                push_error_table(err_table, msg);
            }
        } else if (entry->type->type == FLOAT_t) { 
            if (!is_float_coercion_valid(value->type)) {
                sprintf(msg, "Return type coercion error: %s -> %s",
                        TYPES[get_type_string(value->type->type)], TYPES[get_type_string(entry->type->type)]);
                push_error_table(err_table, msg);
            }
        } else if(entry->type->type != value->type->type) {
            sprintf(msg, "Return type coercion error: %s -> %s",
                    TYPES[get_type_string(value->type->type)], TYPES[get_type_string(entry->type->type)]);
            push_error_table(err_table, msg);
        }
    }
}

void bool_exp_check(struct ErrorTable *err_table, struct Value *expr, const char *use)
{
    if (err_table && expr) {
        if (expr->type->arr != NULL || 
                (expr->type->type != BOOL_t)) {
            char msg[1024];
            if (strcmp(use, "for") == 0)
                sprintf(msg, "Control expression in %s statement is not boolean type", use);
            else
                sprintf(msg, "Condition expression in %s statement is not boolean type", use);

            push_error_table(err_table, msg);
        }
    }
}


int cmp_type(struct Type *lhs, struct Type *rhs)
{
    if (lhs == NULL || rhs == NULL)
        return -1;
    if (lhs->type != rhs->type)
        return 1;

    struct ArrayNode *left_cur = lhs->arr, *right_cur = rhs->arr;
    for (; ;left_cur = left_cur->next, right_cur= right_cur->next) {
        if (left_cur == NULL && right_cur == NULL)
            return 0;
        else if (left_cur == NULL && right_cur != NULL)
            return 1;
        else if (left_cur != NULL && right_cur == NULL)
            return 1;
        else if (left_cur->dim != right_cur->dim)
            return 1;
    }
}
int cmp_type_call(struct Type *lhs, struct Type *rhs)
{
    if (lhs == NULL || rhs == NULL)
        return -1;
    if (lhs->type == DOUBLE_t) 
        if (!is_double_coercion_valid(rhs))
            return 1;
    else if (lhs->type == FLOAT_t) 
            if(!is_float_coercion_valid(rhs))
        return 1;
    else if (lhs->type != rhs->type)
        return 1;
    
    struct ArrayNode *left_cur = lhs->arr, *right_cur = rhs->arr;
    for (; ;left_cur = left_cur->next, right_cur= right_cur->next) {
        if (left_cur == NULL && right_cur == NULL)
            return 0;
        else if (left_cur == NULL && right_cur != NULL)
            return 1;
        else if (left_cur != NULL && right_cur == NULL)
            return 1;
        else if (left_cur->dim != right_cur->dim)
            return 1;
    }
}
int cmp_argu(struct Argu *lhs, struct Argu *rhs)
{
    struct Argu *left_cur = lhs, *right_cur = rhs;
    for (; ; left_cur = left_cur->next, right_cur= right_cur->next) {
        if (left_cur == NULL && right_cur == NULL)
            return 0;
        if (left_cur == NULL && right_cur != NULL)
            return 1;
        if (left_cur != NULL && right_cur == NULL)
            return 1;
        if (cmp_type(left_cur->type, right_cur->type) != 0)
            return 1;
    }
}
int cmp_argu_call(struct Argu *lhs, struct Argu *rhs)
{
    struct Argu *left_cur = lhs, *right_cur = rhs;
    for (; ; left_cur = left_cur->next, right_cur= right_cur->next) {
        if (left_cur == NULL && right_cur == NULL)
            return 0;
        if (left_cur == NULL && right_cur != NULL)
            return 1;
        if (left_cur != NULL && right_cur == NULL)
            return 1;
        if (cmp_type_call(left_cur->type, right_cur->type) != 0)
            return 1;
    }
}


struct Value *expr_neg(struct Value *val, struct ErrorTable *err_table)
{
    if (val) {
        char msg[1024];
        if (val->type->arr != NULL) {
            strncpy(msg, "Invalid array operation with '-'", sizeof(msg));
            push_error_table(err_table, msg);
            del_val(val);
            return NULL;
        }

        if (val->type->type == STRING_t ||
                val->type->type == BOOL_t ||
                val->type->type == BOOLEAN_t) {
            strncpy(msg, "Invalid operation with '-'", sizeof(msg));
            push_error_table(err_table, msg);
            return val;
        } else if (val->type->type == FLOAT_t ||
                val->type->type == DOUBLE_t) {
            val->d_val *= -1;
            return val;
        } else if (val->type->type == INT_t) {
            val->i_val *= -1;
            return val;
        }
    }
    return NULL;
}

struct Value *expr_not(struct Value *val, struct ErrorTable *err_table)
{
    if (val) {
        char msg[1024];
        if (val->type->arr != NULL) {
            strncpy(msg, "Invalid array operation with '!'", sizeof(msg));
            push_error_table(err_table, msg);
            del_val(val);
            return NULL;
        }
        if (val->type->type != BOOL_t && 
                val->type->type != BOOLEAN_t) {
            strncpy(msg, "Invalid operation with '!'", sizeof(msg));
            push_error_table(err_table, msg);
            return val;
        }
        val->i_val = (val->i_val == 0) ? 1 : 0;
        return val;
    } else {
        return NULL;
    }
}

struct Value *expr_add(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, "Add", err_table))
        return NULL;

    expr_evaluate(lhs, rhs, "Add");
    del_val(rhs);
    return lhs;
}
struct Value *expr_sub(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, "Minus", err_table))
        return NULL;

    expr_evaluate(lhs, rhs, "Minus");
    del_val(rhs);
    return lhs;
}
struct Value *expr_mul(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, "Multiply", err_table))
        return NULL;

    expr_evaluate(lhs, rhs, "Multiply");
    del_val(rhs);
    return lhs;
}
struct Value *expr_div(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{    
    if (expr_check_invlaid_type(&lhs, &rhs, "Division", err_table))
        return NULL;

    expr_evaluate(lhs, rhs, "Division");
    del_val(rhs);
    return lhs;
}
struct Value *expr_mod(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type_helper(&lhs, &rhs, "Mod", err_table))
        return NULL;

    char msg[1024];
    if (lhs->type->type != INT_t || 
            rhs->type->type != INT_t) {
        strncpy(msg, "Invalid operation to 'Mod'", sizeof(msg));
        push_error_table(err_table, msg);
        del_val(lhs);
        del_val(rhs);
        return NULL;
    }
    if (rhs->i_val != 0)
        lhs->i_val %= rhs->i_val;

    del_val(rhs);
    return lhs;
}
struct Value *expr_and(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type_logic(&lhs, &rhs, "And", err_table))
        return NULL;

    lhs->i_val = lhs->i_val & rhs->i_val;
    del_val(rhs);
    return lhs;
}
struct Value *expr_or(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type_logic(&lhs, &rhs, "Or", err_table))
        return NULL;

    lhs->i_val = lhs->i_val | rhs->i_val;
    del_val(rhs);
    return lhs;
}
struct Value *expr_lt(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, "<", err_table))
        return NULL;

    logic_expr_evaluate(lhs, rhs, "<");

    del_val(rhs);
    lhs->type->type = BOOL_t;
    return lhs;
}
struct Value *expr_gt(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, ">", err_table))
        return NULL;

    logic_expr_evaluate(lhs, rhs, ">");

    del_val(rhs);
    lhs->type->type = BOOL_t;
    return lhs;
}
struct Value *expr_le(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, "<=", err_table))
        return NULL;

    logic_expr_evaluate(lhs, rhs, "<=");
    del_val(rhs);
    lhs->type->type =  BOOL_t;
    return lhs;
}
struct Value *expr_ge(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, ">=", err_table))
        return NULL;

    logic_expr_evaluate(lhs, rhs, ">=");

    del_val(rhs);
    lhs->type->type = BOOL_t;
    return lhs;
}
struct Value *expr_eq(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, "==", err_table))
        return NULL;

    logic_expr_evaluate(lhs, rhs, "==");

    del_val(rhs);
    lhs->type->type = BOOL_t;
    return lhs;
}
struct Value *expr_ne(struct Value *lhs, struct Value *rhs, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type(&lhs, &rhs, "!=", err_table))
        return NULL;

    logic_expr_evaluate(lhs, rhs, "!=");

    del_val(rhs);
    lhs->type->type = BOOL_t;
    return lhs;
}
