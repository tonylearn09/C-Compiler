#include "helper.h"
#include <string.h>
#include <stdio.h>

extern const char *TYPES[];
extern const char *KINDS[];

bool is_double_coercion_valid(struct Type *type)
{
    if (type->type != DOUBLE_t && 
            type->type != FLOAT_t &&
            type->type != INT_t) {
        return false;
    }
    return true;
}
bool is_float_coercion_valid(struct Type *type)
{
    if(type->type != FLOAT_t &&
            type->type != INT_t) {
        return false;
    }
    return true;
}

bool double_coercion_decl(struct Value *value) 
{
    if (value->type->type == DOUBLE_t)
        ;
    else if (value->type->type == FLOAT_t)
        value->type->type = DOUBLE_t;
    else if (value->type->type == INT_t) {
        value->type->type = DOUBLE_t;
        value->d_val = (double) value->i_val;
    } else {
        return false;
    }
    return true;
}

bool float_coercion_decl(struct Value *value)
{
    if (value->type->type == FLOAT_t)
        ;
    else if (value->type->type == INT_t) {
        value->type->type = FLOAT_t;
        value->d_val = (double) value->i_val;
    } else {
        return false;
    }
    return true;
}

void decl_defaul_value(struct Entry *entry, struct Value **value)
{
    if (entry->type->type == DOUBLE_t ||
            entry->type->type == FLOAT_t ||
            entry->type->type == INT_t)
        *value = build_value(copy_type(entry->type), "0.0");
    else if (entry->type->type == BOOL_t ||
            entry->type->type == BOOLEAN_t)
        *value = build_value(copy_type(entry->type), "false");
    else
        *value = build_value(copy_type(entry->type), "");
}

void print_invlaid_initial_err(char *msg, struct Attribute **attr, struct ErrorTable *err_table)
{
    sprintf(msg, "Invalid initial value type: %s", TYPES[get_type_string((*attr)->value->type->type)]);
    push_error_table(err_table, msg);
    del_attr(*attr);
    *attr= NULL;
}

void print_invlaid_initial_corecion_err(char *msg, struct Attribute **attr, struct Entry *entry, struct ErrorTable *err_table)
{
    sprintf(msg, "Invalid initial value type coercion: %s -> %s",
            TYPES[get_type_string((*attr)->value->type->type)], TYPES[get_type_string(entry->type->type)]);
    push_error_table(err_table, msg);
    del_attr(*attr);
    *attr= NULL;
}

void print_invalid_initial_array_coercion_err(char *msg, struct Value **value, struct Entry *entry, struct ErrorTable *err_table)
{
    sprintf(msg, "Invalid initial value type coercion: %s -> %s",
            TYPES[get_type_string((*value)->type->type)], TYPES[get_type_string(entry->type->type)]);
    push_error_table(err_table, msg);
    del_val(*value);
}

int get_array_size(struct ArrayNode *cur, char *msg, struct ErrorTable *err_table)
{
    int array_size = 1;
    while (cur != NULL) {
        array_size *= (cur->dim);
        cur = cur->next;
    }
    if (array_size <= 0) {
        sprintf(msg, "The index must be greater than zero in array declaration");
        push_error_table(err_table, msg);
    }

    return array_size;
}

bool expr_check_invlaid_type_helper(struct Value **lhs, struct Value **rhs, const char *operator, struct ErrorTable *err_table)
{
    if (*lhs == NULL || *rhs == NULL) {
        del_val(*lhs);
        del_val(*rhs);
        return true;
    }

    char msg[1024];
    if ((*lhs)->type->arr != NULL || (*rhs)->type->arr != NULL) {
        sprintf(msg, "Invalid array operation to '%s'", operator);
        push_error_table(err_table, msg);
        del_val(*lhs);
        del_val(*rhs);
        return true;
    }
    return false;
}

bool expr_check_invlaid_type_logic(struct Value **lhs, struct Value **rhs, const char *operator, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type_helper(&(*lhs), &(*rhs), operator, err_table))
        return true;

    char msg[1024];
    if (((*lhs)->type->type != BOOL_t &&
            (*lhs)->type->type != BOOLEAN_t) ||
            ((*rhs)->type->type != BOOL_t &&
            (*rhs)->type->type != BOOLEAN_t)) {
        sprintf(msg, "Invalid operation to '%s'", operator);
        push_error_table(err_table, msg);
        del_val(*lhs);
        del_val(*rhs);
        return true;
    }
    return false;
}

bool expr_check_invlaid_type(struct Value **lhs, struct Value **rhs, const char *operator, struct ErrorTable *err_table)
{
    if (expr_check_invlaid_type_helper(&(*lhs), &(*rhs), operator, err_table))
        return true;

    char msg[1024];
    if (strcmp(operator, "==") == 0 || strcmp(operator, "!=") == 0) {
        if ((*lhs)->type->type == STRING_t || 
                (*rhs)->type->type == STRING_t) {
            sprintf(msg, "Invalid operation to '%s'", operator);
            push_error_table(err_table, msg);
            del_val(*lhs);
            del_val(*rhs);
            return true;
        }
    } else {
        if ((*lhs)->type->type == STRING_t || 
                (*lhs)->type->type == BOOL_t ||
                (*lhs)->type->type == BOOLEAN_t ||
                (*rhs)->type->type == STRING_t ||
                (*rhs)->type->type == BOOL_t ||
                (*rhs)->type->type == BOOLEAN_t) {
            sprintf(msg, "Invalid operation to '%s'", operator);
            push_error_table(err_table, msg);
            del_val(*lhs);
            del_val(*rhs);
            return true;
        }
    }
    return false;
}

void expr_evaluate(struct Value *lhs, struct Value *rhs, const char *operator)
{
    if (lhs->type->type == DOUBLE_t) {
        if (rhs->type->type == INT_t) {
            if (strcmp(operator, "Add") == 0) {
                lhs->d_val += (double) rhs->i_val;
            } else if (strcmp(operator, "Minus") == 0) {
                lhs->d_val -= (double) rhs->i_val;
            } else if (strcmp(operator, "Multiply") == 0) {
                lhs->d_val *= (double) rhs->i_val;
            } else if (strcmp(operator, "Division") == 0) {
                if (rhs->i_val != 0)
                    lhs->d_val /= (double) rhs->i_val;
            }
        }
        else {
            if (strcmp(operator, "Add") == 0) {
                lhs->d_val += rhs->d_val;
            } else if (strcmp(operator, "Minus") == 0) {
                lhs->d_val -= rhs->d_val;
            } else if (strcmp(operator, "Multiply") == 0) {
                lhs->d_val *= rhs->d_val;
            } else if (strcmp(operator, "Division") == 0) {
                if (rhs->d_val != 0)
                    lhs->d_val /= rhs->d_val;
            }
        }
    } else if (lhs->type->type == FLOAT_t) {
        if (rhs->type->type == INT_t) {
            if (strcmp(operator, "Add") == 0) {
                lhs->d_val += (double) rhs->i_val;
            } else if (strcmp(operator, "Minus") == 0) {
                lhs->d_val -= (double) rhs->i_val;
            } else if (strcmp(operator, "Multiply") == 0) {
                lhs->d_val *= (double) rhs->i_val;
            } else if (strcmp(operator, "Division") == 0) {
                if (rhs->i_val != 0)
                    lhs->d_val /= (double) rhs->i_val;
            }
        }
        else {
            if (strcmp(operator, "Add") == 0) {
                lhs->d_val += rhs->d_val;
                lhs->type->type = rhs->type->type;
            } else if (strcmp(operator, "Minus") == 0) {
                lhs->d_val -= rhs->d_val;
                lhs->type->type =  rhs->type->type;
            } else if (strcmp(operator, "Multiply") == 0) {
                lhs->d_val *= rhs->d_val;
                lhs->type->type = rhs->type->type;
            } else if (strcmp(operator, "Division") == 0) {
                if (rhs->d_val != 0) {
                    lhs->d_val /= rhs->d_val;
                    lhs->type->type = rhs->type->type;
                }
            }
        }
    } else if (lhs->type->type == INT_t) {
        if (rhs->type->type == INT_t) {
            if (strcmp(operator, "Add") == 0) {
                lhs->i_val += rhs->i_val;
            } else if (strcmp(operator, "Minus") == 0) {
                lhs->i_val -= rhs->i_val;
            } else if (strcmp(operator, "Multiply") == 0) {
                lhs->i_val *= rhs->i_val;
            } else if (strcmp(operator, "Division") == 0) {
                if (rhs->i_val != 0) {
                    lhs->i_val /= rhs->i_val;
                }
            }
        }
        else {
            if (strcmp(operator, "Add") == 0) {
                lhs->d_val = (double)lhs->i_val - rhs->d_val;
                lhs->type->type = rhs->type->type;
            } else if (strcmp(operator, "Minus") == 0) {
                lhs->d_val = (double)lhs->i_val - rhs->d_val;
                lhs->type->type = rhs->type->type;
            } else if (strcmp(operator, "Multiply") == 0) {
                lhs->d_val = (double)lhs->i_val - rhs->d_val;
                lhs->type->type = rhs->type->type;
            } else if (strcmp(operator, "Division") == 0) {
                if (rhs->d_val != 0) {
                    lhs->d_val = (double)lhs->i_val / rhs->d_val;
                    lhs->type->type = rhs->type->type;
                }
            }
        }
    }
}

void logic_expr_evaluate(struct Value *lhs, struct Value *rhs, char *operator)
{
    bool flag;
    if (lhs->type->type == DOUBLE_t ||
            lhs->type->type == FLOAT_t) {
        if (rhs->type->type == INT_t) {

            if (strcmp(operator, "<") == 0)
                flag = (lhs->d_val < rhs->i_val);
            else if (strcmp(operator, ">") == 0)
                flag = (lhs->d_val > rhs->i_val);
            else if (strcmp(operator, ">=") == 0)
                flag = (lhs->d_val >= rhs->i_val);
            else if (strcmp(operator, "<=") == 0)
                flag = (lhs->d_val <= rhs->i_val);
            else if (strcmp(operator, "==") == 0)
                flag = (lhs->d_val == rhs->i_val);
            else if (strcmp(operator, "!=") == 0)
                flag = (lhs->d_val != rhs->i_val);

            if (flag)
                lhs->i_val = 1;
            else
                lhs->i_val = 0;
        }
        else {
            if (strcmp(operator, "<") == 0)
                flag = (lhs->d_val < rhs->d_val);
            else if (strcmp(operator, ">") == 0)
                flag = (lhs->d_val > rhs->d_val);
            else if (strcmp(operator, ">=") == 0)
                flag = (lhs->d_val >= rhs->d_val);
            else if (strcmp(operator, "<=") == 0)
                flag = (lhs->d_val <= rhs->d_val);
            else if (strcmp(operator, "==") == 0)
                flag = (lhs->d_val == rhs->d_val);
            else if (strcmp(operator, "!=") == 0)
                flag = (lhs->d_val != rhs->d_val);

            if (flag)
                lhs->i_val = 1;
            else
                lhs->i_val = 0;
        }
    } else if (lhs->type->type == INT_t) {
        if (rhs->type->type == INT_t) {
            if (strcmp(operator, "<") == 0)
                flag = (lhs->i_val < rhs->i_val);
            else if (strcmp(operator, ">") == 0)
                flag = (lhs->i_val > rhs->i_val);
            else if (strcmp(operator, ">=") == 0)
                flag = (lhs->i_val >= rhs->i_val);
            else if (strcmp(operator, "<=") == 0)
                flag = (lhs->i_val <= rhs->i_val);
            else if (strcmp(operator, "==") == 0)
                flag = (lhs->i_val == rhs->i_val);
            else if (strcmp(operator, "!=") == 0)
                flag = (lhs->i_val != rhs->i_val);

            if (flag)
                lhs->i_val = 1;
            else
                lhs->i_val = 0;
        } else {
            if (strcmp(operator, "<") == 0)
                flag = (lhs->i_val < rhs->d_val);
            else if (strcmp(operator, ">") == 0)
                flag = (lhs->i_val > rhs->d_val);
            else if (strcmp(operator, ">=") == 0)
                flag = (lhs->i_val >= rhs->d_val);
            else if (strcmp(operator, "<=") == 0)
                flag = (lhs->i_val <= rhs->d_val);
            else if (strcmp(operator, "==") == 0)
                flag = (lhs->i_val == rhs->d_val);
            else if (strcmp(operator, "!=") == 0)
                flag = (lhs->i_val != rhs->d_val);

            if (flag)
                lhs->i_val = 1;
            else
                lhs->i_val = 0;
        }
    }
}
