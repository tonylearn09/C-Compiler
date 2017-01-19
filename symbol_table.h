#ifndef __SYMBOL_TABLE_H
#define __SYMBOL_TABLE_H

#include <stdbool.h>
typedef enum {
    VARIABLE_t = 0,
    CONSTANT_t,
    FUNCTION_t,
    PARAMETER_t,
    INITIAL_t
} kind_type;

typedef enum {
    INT_t = 0,
    FLOAT_t,
    DOUBLE_t,
    STRING_t,
    BOOL_t,
    BOOLEAN_t,
    VOID_t
    //UNKNOWN
} type_type;

typedef enum {
    LESS,
    LESS_EQ,
    GREAT,
    GREAT_EQ,
    EQUAL,
    NEQUAL,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NOT,
    AND,
    OR
} Operator_Type;

//const char *TYPES[] = {"Int", "Float" "Double", "String", "Bool", "Boolean", "Void"};
//const char *KINDS[] = {"Variable", "Constant", "Function", "Parameter"};


struct ErrorTable {
    int size;
    int capacity;
    struct ErrorEntry **errors;
};

struct ErrorEntry {
    int linenum;
    char msg[1024];
};

struct SymbolTable {
    int size;
    int capacity;
    int cur_level;
    struct Entry **entries;
};

struct Entry {
    bool decl; // for function
    kind_type kind;
    char name[33];
    int level;
    int location;
    struct Type *type;
    struct Attribute *attr;
};

struct Type {
    type_type type;
    struct ArrayNode *arr;
};

struct ArrayNode {
    int dim;
    struct ArrayNode *next;
};

struct Attribute {
    struct Argu *argu;
    struct Value *value;
    struct ValueArray *v_array;
};

struct Argu {
    char name[33];
    struct Type *type;
    struct Argu *next;
};

struct Value {
    struct Type *type;
    char string_literal[1024];
    double d_val;
    int i_val;
};

struct ValueArray {
    int size;
    int capacity;
    struct Value **values;
};

struct Label {
    int label_num;
    int label_type;
};

struct LabelStack {
    int size;
    int capacity;
    struct Label **labels;
};


int get_type_string(type_type t);
int get_kind_string(kind_type k);

// Symbol table related
struct SymbolTable *build_symbol_table();
void print_symbol_table(struct SymbolTable*);
void print_level(int);
void print_type(struct Type *, bool);
void print_attr(struct Attribute *, kind_type, struct Type *);

void enlarge_capacity(struct SymbolTable *);
void insert_entry(struct SymbolTable *, struct Entry *);
void insert_argu(struct SymbolTable *, struct Argu *, int, struct ErrorTable *);
void pop_symbol_table(struct SymbolTable *);
struct Entry *look_up_entry(struct SymbolTable *, const char *);
struct Entry *find_id(struct SymbolTable *, struct ErrorTable *, const char *);


// Error table related
void enlarge_capacity_err(struct ErrorTable *);
struct ErrorTable *build_error_table();
void print_error_table(struct ErrorTable*);
void push_error_table(struct ErrorTable *, const char *);

// Entry related
struct Entry *build_entry(const char *, kind_type, int, struct Type *,
        struct Attribute *);
void assign_entry(struct Entry *, const char *, kind_type, int,
        struct Type *, struct Attribute *);
struct Entry *copy_entry(const struct Entry*);
struct Entry *clear_entry(struct Entry *);


// Type related
struct Type *build_type(type_type, struct ArrayNode *);
struct Type *copy_type(const struct Type *);

// Attribute related
struct Attribute *build_attribute(struct Argu *, struct Value *);
struct Attribute *copy_attribute(const struct Attribute *);

// Argument related
struct Argu *build_argu(struct Type *);
struct Argu *copy_argu(struct Argu *);
void add_argu(struct Argu **, const char *, struct Type *);

// Value related
struct Value *build_value(struct Type *, const char *);
struct Value *build_default_value(struct Type *);
struct Value *copy_value(struct Value *);
void init_value(struct Entry *, struct Value *);

// ValueArray related
struct ValueArray *build_valuearray();
struct ValueArray *copy_valuearray(struct ValueArray *);
void insert_valuearray(struct ValueArray *, struct Value *);
void assign_valuearray(struct Entry *, struct ValueArray *);


// Arraynode related
struct Type *reduce_type_array(struct Type *, int, struct ErrorTable *);
struct ArrayNode *copy_array(struct ArrayNode *);
void add_dim(struct ArrayNode **, int);

// LabelStack related
struct LabelStack *build_label_stack();
void push_to_stack(int label_num, int label_type);
void enlarge_capacity_label_stack(struct LabelStack *);
int pop_stack();
int get_stack_top(int label_type);

// Delete related
void del_symbol_table(struct SymbolTable *);
void del_error_table(struct ErrorTable *);
void del_entry(struct Entry*);
void del_type(struct Type *);
void del_attr(struct Attribute *);
void del_val(struct Value *);
void del_valarray(struct ValueArray *);
void del_array(struct ArrayNode **);
void del_argu(struct Argu **);
void del_label_stack(struct LabelStack *);

void print_statistic();


#endif
