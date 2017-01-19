#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symbol_table.h"
#include "codegen.h"

#define INITIAL_SIZE_OF_TABLE 32 

extern int linenum; // lex
extern struct ErrorTable *error_msg; // parser
extern struct LabelStack *label_stack;

const char *TYPES[] = {"int", "float", "double", "string", "bool", "boolean", "void"};
const char *KINDS[] = {"variable", "constant", "function", "parameter", "no kind"};

int get_kind_string(kind_type k)
{
    return (int)k;
}

int get_type_string(type_type t)
{
    return (int)t;
}

struct SymbolTable *build_symbol_table()
{
    struct SymbolTable *sym_table = (struct SymbolTable *)malloc(sizeof(struct SymbolTable));
    sym_table->cur_level = 0; // global
    sym_table->size = 0;
    sym_table->capacity = INITIAL_SIZE_OF_TABLE;
    sym_table->entries = (struct Entry **)malloc(sizeof(struct Entry *) * INITIAL_SIZE_OF_TABLE);
    return sym_table;
}

void print_symbol_table(struct SymbolTable *sym_table)
{
    printf("===================================================================================================\n");
    printf("Name                             Kind       Level       Type               Attribute               \n");
    printf("---------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < sym_table->size; ++i) {
        if (sym_table->entries[i]->level == sym_table->cur_level) {
            printf("%-33s", sym_table->entries[i]->name);
            printf("%-11s", KINDS[get_kind_string(sym_table->entries[i]->kind)]);
            print_level(sym_table->entries[i]->level);
            print_type(sym_table->entries[i]->type, false);
            print_attr(sym_table->entries[i]->attr, sym_table->entries[i]->kind, sym_table->entries[i]->type);
            printf("\n");

        }
    }
    printf("===================================================================================================\n\n");
}

void print_level(int level)
{
    char buf[15];
    sprintf(buf, "%d", level);
    if (level == 0) {
        strcat(buf, "(global)");
        printf("%-12s", buf);
    } else {
        strcat(buf, "(local)");
        printf("%-12s", buf);
    }
}

void print_type(struct Type *ele_type, bool attr_type)
{
    // Not empty element
    if (ele_type) {
        char buf[1024];
        strncpy(buf, TYPES[get_type_string(ele_type->type)], sizeof(buf));

        struct ArrayNode *temp = ele_type->arr;
        char dim_buf[16];
        while (temp) {
            // if it is array
            sprintf(dim_buf, "[%d]", temp->dim);
            strcat(buf, dim_buf);
            temp = temp->next;
        }
        if (attr_type)  // if it is printing attribute 
            printf("%s", buf); // At the end, no restrict
        else
            printf("%-19s", buf);
    }
}

void print_attr(struct Attribute *attribute, kind_type kind, struct Type *ele_type)
{
    if (attribute) {
        if (kind == CONSTANT_t) {
            if (ele_type->type == INT_t)
                printf("%d", attribute->value->i_val);
            else if (ele_type->type == DOUBLE_t ||
                    ele_type->type == FLOAT_t)
                printf("%lf", attribute->value->d_val);
            else if (ele_type->type == BOOL_t ||
                    ele_type->type == BOOLEAN_t) {
                if (attribute->value->i_val)
                    printf("true");
                else
                    printf("false");
            }
            else if (ele_type->type == STRING_t) {
                printf("%s", attribute->value->string_literal);
            }
        } else if (kind == FUNCTION_t) {
            struct Argu *temp = attribute->argu;
            bool begining = true;
            while (temp) {
                if (!begining)
                    printf(",");
                print_type(temp->type, true);

                begining = false;
                temp = temp->next;
            }
        } else
            ;
    }
}

void enlarge_capacity(struct SymbolTable *sym_table)
{
    sym_table->capacity *= 2;
    struct Entry **old_entries = sym_table->entries;
    sym_table->entries = (struct Entry **)malloc(sizeof(struct Entry *) * sym_table->capacity);
    for (int i = 0; i < sym_table->size; ++i)
        (sym_table->entries)[i] = old_entries[i];
    free(old_entries);
}

void insert_entry(struct SymbolTable *sym_table, struct Entry *element)
{
    if (sym_table && element) {
        if (sym_table->capacity == sym_table->size) {
            enlarge_capacity(sym_table);
        }
        sym_table->entries[(sym_table->size)++] = element;
    }
}


struct Entry *look_up_cur_scope(struct SymbolTable *sym_table, struct Entry *entry, int cur_scope, struct ErrorTable *err_table)
{
    //int size = sym_table->size;
    for (int i = sym_table->size - 1; i >= 0; --i) {
        if (strcmp(sym_table->entries[i]->name, entry->name) == 0 && 
                sym_table->entries[i]->level == cur_scope) {
            char msg[1024];
            strncpy(msg, "Identifier redeclared in argument", sizeof(msg));
            push_error_table(err_table, msg);
            return sym_table->entries[i];
        }
    }
    return NULL;
}

void insert_argu(struct SymbolTable *sym_table, struct Argu *argu, int cur_scope, struct ErrorTable *err_table) {
    if (sym_table) {
        struct SymbolTable *sym_table_buf = build_symbol_table();
        struct Argu *temp;
        for (temp = argu; temp != NULL; temp = temp->next) {
            // Argument has no attribute, so is NULL
            struct Entry *invalid = look_up_cur_scope(sym_table, build_entry(temp->name, PARAMETER_t,
                        sym_table->cur_level, copy_type(temp->type), NULL), cur_scope, err_table);
            if (!invalid) {
                struct Entry *argu_entry = build_entry(temp->name, PARAMETER_t,
                        sym_table->cur_level, copy_type(temp->type), NULL);
                gen_var_decl(argu_entry);
                /*insert_entry(sym_table, build_entry(temp->name, PARAMETER_t,
                            sym_table->cur_level, copy_type(temp->type), NULL));*/
                insert_entry(sym_table, argu_entry);
            }
        }
    }
}

void pop_symbol_table(struct SymbolTable *sym_table)
{
    int size = sym_table->size;
    for (int i = 0; i < size; ++i) {
        if (sym_table->entries[i]->level == sym_table->cur_level) {
            del_entry(sym_table->entries[i]);
            sym_table->entries[i] = NULL;
            sym_table->size--;
        }
    }
    sym_table->cur_level--;
}


struct Entry *look_up_entry(struct SymbolTable *sym_table, const char *id)
{
    //int size = sym_table->size;
    for (int i = sym_table->size - 1; i >= 0; --i) {
        if (strcmp(sym_table->entries[i]->name, id) == 0) {
            return sym_table->entries[i];
        }
    }
    return NULL;
}


struct Entry *find_id(struct SymbolTable *sym_table, struct ErrorTable *err_table, const char *id)
{
    struct Entry *finded_entry = look_up_entry(sym_table, id);
    if (!finded_entry) {
        char msg[1024];
        sprintf(msg, "No Identifier '%s'", id);
        push_error_table(err_table, msg);
    }
    return finded_entry;
}


struct ErrorTable *build_error_table()
{
    struct ErrorTable *err_table = (struct ErrorTable *) malloc(sizeof(struct ErrorTable));
    err_table->capacity = INITIAL_SIZE_OF_TABLE;
    err_table->size = 0;
    err_table->errors = (struct ErrorEntry **) malloc(sizeof(struct ErrorEntry *) * INITIAL_SIZE_OF_TABLE);
    return err_table;
}

void print_error_table(struct ErrorTable *err_table)
{
    if (err_table) {
        //printf("\n");
        //int size = err_table->size;
        for (int i = 0; i < err_table->size; ++i)
            printf("##########Error at Line #%-5d: %s. ##########\n", 
                    err_table->errors[i]->linenum, err_table->errors[i]->msg);
    }
}

void enlarge_capacity_err(struct ErrorTable *err_table)
{
    err_table->capacity *= 2;
    struct ErrorEntry **old_error = err_table->errors;
    err_table->errors = (struct ErrorEntry **) malloc(sizeof(struct ErrorEntry *) * err_table->capacity);
    for (int i = 0; i < err_table->size; ++i)
        (err_table->errors)[i] = old_error[i];
    free(old_error);
}

void push_error_table(struct ErrorTable *err_table, const char *msg)
{
    if (err_table) {
        if (err_table->size == err_table->capacity) {
            enlarge_capacity_err(err_table);
        }

        struct ErrorEntry *new_error = (struct ErrorEntry *)malloc(sizeof(struct ErrorEntry));
        new_error->linenum = linenum;
        strncpy(new_error->msg, msg, sizeof(new_error->msg));
        err_table->errors[(err_table->size)++] = new_error;
    }
}


struct Entry *build_entry(const char *name, kind_type kind, int level, struct Type *type, struct Attribute *attr)
{
    struct Entry *new_entry = (struct Entry *)malloc(sizeof(struct Entry));
    memset(new_entry, 0, sizeof(struct Entry));
    new_entry->decl = false;
    assign_entry(new_entry, name, kind, level, type, attr);
    return new_entry;
}

void assign_entry(struct Entry *new_entry, const char *name, kind_type kind, int level, struct Type *type, struct Attribute *attr)
{
    if (new_entry) {
        if (name) {
            strncpy(new_entry->name, name, sizeof(new_entry->name));
        }
        //if (kind) 
            new_entry->kind = kind;
        if (level >= 0) 
            new_entry->level = level;
        if (type) {
            if (new_entry->type)
                del_type(new_entry->type);
            new_entry->type= type;
        }
        if (attr) {
            if (new_entry->attr)
                del_attr(new_entry->attr);
            new_entry->attr = attr;
        } else if (kind != FUNCTION_t && type) {
            // attr is NULL, and is not function
            new_entry->attr =
                build_attribute(NULL, build_default_value(copy_type(type)));
        }
    }
}
struct Entry *copy_entry(const struct Entry* temp)
{
    if (temp) {
        struct Entry *new_entry = (struct Entry *)malloc(sizeof(struct Entry));
        memset(new_entry, 0, sizeof(struct Entry));
        assign_entry(new_entry, temp->name, temp->kind, temp->level, 
                copy_type(temp->type), copy_attribute(temp->attr));
        new_entry->location = temp->location;
        return new_entry;
    } else {
        return NULL;
    }
}
struct Entry *clear_entry(struct Entry *temp)
{
    strcpy(temp->name, "");
    temp->kind = INITIAL_t;
    temp->level = 0;
    del_attr(temp->attr);
    temp->attr = NULL;
    // Added below
    del_type(temp->type);
    temp->type = NULL;
}


struct Type *build_type(type_type type, struct ArrayNode *arr)
{
    struct Type *new_type = (struct Type*) malloc(sizeof(struct Type));
    new_type->type = type;
    new_type->arr = arr;
    return new_type;
}
struct Type *copy_type(const struct Type *temp)
{
    if (temp) {
        struct Type *new_type = build_type(temp->type, NULL);
        for (struct ArrayNode *cur = temp->arr; cur != NULL; cur = cur->next) {
            add_dim(&(new_type->arr), cur->dim);
        }
        return new_type;
    } else {
        return NULL;
    }
}

// Attribute related
struct Attribute *build_attribute(struct Argu *argu, struct Value *val)
{
    struct Attribute *new_attr = (struct Attribute*) malloc(sizeof(struct Attribute));
    new_attr->argu = argu;
    new_attr->value = val;
    new_attr->v_array = NULL;
    return new_attr;
}
struct Attribute *copy_attribute(const struct Attribute *temp)
{
    if (temp) {
        struct Attribute *new_attr = 
            build_attribute(copy_argu(temp->argu), copy_value(temp->value));
        new_attr->v_array = copy_valuearray(temp->v_array);
        return new_attr;
    } else 
        return NULL;
}


struct Argu *build_argu(struct Type *type)
{
    if (type) {
        struct Argu *new_argu = (struct Argu*)malloc(sizeof(struct Argu));
        new_argu->type = type;
        new_argu->next = NULL;
        return new_argu;
    } else {
        return NULL;
    }
}
struct Argu *copy_argu(struct Argu *temp)
{
    if (temp) {
        struct Argu *new_argu = build_argu(copy_type(temp->type));
        strncpy(new_argu->name, temp->name, sizeof(new_argu->name));
        for (struct Argu *cur = temp->next; cur != NULL; cur = cur->next)
            add_argu(&new_argu, cur->name, cur->type);
        return new_argu;
    } else {
         return NULL;
    }
}
void add_argu(struct Argu **head, const char *name, struct Type *type)
{
    struct Argu *new_argu = build_argu(copy_type(type));
    strncpy(new_argu->name, name, sizeof(new_argu->name));
    if (*head) {
        struct Argu *cur, *prev;
        for (cur = *head, prev = NULL; cur != NULL; prev= cur, cur = cur->next) 
            ;
        prev->next = new_argu;
    } else {
        *head = new_argu;
    }
}

// Value related
struct Value *build_value(struct Type *type, const char *val)
{
    struct Value *new_value = (struct Value *)malloc(sizeof(struct Value));
    new_value->type = type;
    if (type->type == INT_t) {
        new_value->i_val = atoi(val);
    } else if (type->type == FLOAT_t || 
            type->type == DOUBLE_t) {
        new_value->d_val = atof(val);
    } else if (type->type == BOOL_t ||
            type->type ==  BOOLEAN_t) {
        if (strcmp(val, "true") == 0)
            new_value->i_val = 1;
        else if (strcmp(val, "false") == 0)
            new_value->i_val = 0;
    } else if (type->type == STRING_t) {
        strncpy(new_value->string_literal, val, sizeof(new_value->string_literal));
    }
    return new_value;
}
struct Value *build_default_value(struct Type *type)
{
    if (type) {
        //struct Value *default_value = NULL;
        struct Value *default_value;
        if (type->type == STRING_t)
            default_value = build_value(type, "");
        else if (type->type == BOOL_t || type->type == BOOLEAN_t)
            default_value = build_value(type, "false");
        else // int, float, double
            default_value = build_value(type, "0");
        return default_value;
    } else {
        return NULL;
    }
}
struct Value *copy_value(struct Value *temp)
{
    if (temp) {
        struct Value *new_value = (struct Value *)malloc(sizeof(struct Value));
        new_value->type = copy_type(temp->type);
        strncpy(new_value->string_literal, temp->string_literal, sizeof(new_value->string_literal));
        new_value->i_val = temp->i_val;
        new_value->d_val = temp->d_val;
        return new_value;
    } else {
        return NULL;
    }
}

void init_value(struct Entry *new_entry, struct Value *initial_value)
{
    // For constant
    if (new_entry && initial_value) {
        del_attr(new_entry->attr);
        new_entry->attr = build_attribute(NULL, initial_value);
    }
}


struct ValueArray *build_valuearray()
{
    struct ValueArray *new_valuearray = (struct ValueArray *)malloc(sizeof(struct ValueArray));
    new_valuearray->size = 0;
    new_valuearray->capacity = INITIAL_SIZE_OF_TABLE;
    new_valuearray->values = (struct Value **)malloc(sizeof(struct Value *) * INITIAL_SIZE_OF_TABLE);
    return new_valuearray;
}
struct ValueArray *copy_valuearray(struct ValueArray *temp)
{
    if (temp) {
        struct ValueArray *new_valuearray = (struct ValueArray *)malloc(sizeof(struct ValueArray));
        new_valuearray->size = temp->size;
        new_valuearray->capacity = temp->capacity;
        new_valuearray->values = (struct Value **)malloc(sizeof(struct Value *) * new_valuearray->capacity);
        for (int i = 0; i < new_valuearray->size; ++i) {
            new_valuearray->values[i] = copy_value(temp->values[i]);
        }
        return new_valuearray;
    } else {
        return NULL;
    }
}
void insert_valuearray(struct ValueArray *head, struct Value *new_val)
{
    if (head) {
        if (head->size == head->capacity) {
            head->capacity *= 2;
            struct Value **old_values = head->values;
            head->values = (struct Value **)malloc(sizeof(struct Value *) * head->capacity);
            for (int i = 0; i < head->size; ++i)
                (head->values)[i] = old_values[i];
            free(old_values);
        }
        head->values[(head->size)++] = new_val;
    }
}
void assign_valuearray(struct Entry *entry, struct ValueArray *valuearray)
{
    if (entry && valuearray) {
        if (entry->attr == NULL)
            entry->attr = build_attribute(NULL, NULL);
        del_val(entry->attr->value);
        entry->attr->value = NULL;
        del_valarray(entry->attr->v_array);
        entry->attr->v_array = valuearray;
    }
}

struct Type *reduce_type_array(struct Type *type, int times, struct ErrorTable *err_table)
{
    if (type) {
        for (int i = 0; i < times; ++i) {
            if (type->arr == NULL) {
                char msg[1024];
                strncpy(msg, "Invalid accesst to array", sizeof(msg));
                push_error_table(err_table, msg);
                return NULL;
            }
            struct ArrayNode *old_arraynode = type->arr;
            type->arr = type->arr->next;
            free(old_arraynode);
        }
        return type;
    } else {
        return NULL;
    }
}
struct ArrayNode *copy_array(struct ArrayNode *temp)
{
    struct ArrayNode *new_arraynode = NULL;
    for (struct ArrayNode *cur = temp; cur != NULL; cur = cur->next) {
        add_dim(&new_arraynode, cur->dim);
    }
    return new_arraynode;
}

void add_dim(struct ArrayNode ** head, int dimension)
{
    struct ArrayNode *new_arraynode = (struct ArrayNode *)malloc(sizeof(struct ArrayNode));
    new_arraynode->dim = dimension;
    new_arraynode->next = NULL;
    if (*head) {
        struct ArrayNode *cur = *head, *prev= NULL;
        for (; cur; prev = cur, cur = cur->next)
            ;
        prev->next = new_arraynode;
    } else {
        *head = new_arraynode;
    }
}

// LabelStack related
struct LabelStack *build_label_stack()
{
    struct LabelStack *label_stack = (struct LabelStack *)malloc(sizeof(struct LabelStack));
    label_stack->capacity = INITIAL_SIZE_OF_TABLE; // Need to change this back
    label_stack->size = 0;
    label_stack->labels = (struct Label **)malloc(sizeof(struct Label *) * INITIAL_SIZE_OF_TABLE);
    return label_stack;
}

void push_to_stack(int label_num, int label_type)
{
    if (label_stack) {
        if (label_stack->size == label_stack->capacity) {
            enlarge_capacity_label_stack(label_stack);
        }

        struct Label *new_label = (struct Label *)malloc(sizeof(struct Label));
        new_label->label_num = label_num;
        new_label->label_type = label_type;
        (label_stack->labels)[label_stack->size] = new_label;
        (label_stack->size)++;
    }

}

void enlarge_capacity_label_stack(struct LabelStack *label_stack)
{
    (label_stack->capacity) *= 2;
    struct Label **old_labels = label_stack->labels;
    label_stack->labels = (struct Label **) malloc(sizeof(struct Label *) * label_stack->capacity);
    for (int i = 0; i < label_stack->size; ++i)
        (label_stack->labels)[i] = old_labels[i];
    free(old_labels);
}

int pop_stack()
{
    if (label_stack) {
        if (label_stack->size == 0) {
            return -1; // error
        } else {
            return ((label_stack->labels)[--(label_stack->size)])->label_num;
        }
    }
}

int get_stack_top(int label_type)
{
    for (int i = label_stack->size - 1; i > -1; --i) {
        if (((label_stack->labels)[i])->label_type == label_type) {
            return ((label_stack->labels)[i])->label_num;
        }
    }
    return -2; //error
}


void del_symbol_table(struct SymbolTable *sym_table)
{
    if (sym_table) {
        //int size = sym_table->size;
        for (int i = 0; i < sym_table->size; ++i)
            del_entry(sym_table->entries[i]);
        free(sym_table->entries);
        sym_table->size = 0;
    }
}
void del_error_table(struct ErrorTable *err_table)
{
    if (err_table) {
        //int size = err_table->size;
        for (int i = 0; i < err_table->size; ++i)
            free(err_table->errors[i]);
        free(err_table->errors);
        err_table->size = 0;
    }
}

void del_label_stack(struct LabelStack *label_stack)
{
    if (label_stack) {
        for (int i = 0; i < label_stack->size; ++i) 
            free((label_stack->labels)[i]);
        free(label_stack->labels);
        label_stack->size = 0;
    }
}

void del_entry(struct Entry *entry)
{
    if (entry) {
        del_type(entry->type);
        del_attr(entry->attr);
        free(entry);
    }
}
void del_type(struct Type *type)
{
    if (type) {
        struct ArrayNode *cur = type->arr, *temp = NULL;
        while (cur) {
            temp = cur;
            cur = cur->next;
            free(temp);
        }
        free(type);
    }
}
void del_attr(struct Attribute *attr)
{
    if (attr) {
        del_argu(&(attr->argu));
        del_val(attr->value);
        del_valarray(attr->v_array);
        free(attr);
    }
}
void del_val(struct Value *value)
{
    if (value) {
        del_type(value->type);
        free(value);
    }
}
void del_valarray(struct ValueArray *valuearray)
{
    if (valuearray) {
        //int size = valuearray->size;
        for (int i = 0; i < valuearray->size; ++i) 
            del_val(valuearray->values[i]);

        free(valuearray->values);
        valuearray->size = 0;
    }
}
void del_array(struct ArrayNode **head)
{
    struct ArrayNode *cur = *head, *temp = NULL;
    while (cur != NULL) {
        temp = cur;
        cur = cur->next;
        free(temp);
    }
    *head = NULL;
}

void del_argu(struct Argu **head)
{
    struct Argu *cur = *head, *temp = NULL;
    while (cur != NULL) {
        temp = cur;
        cur = cur->next;
        free(temp);
    }
    *head = NULL;
}
