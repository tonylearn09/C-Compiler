#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

#define TEMP_STACK_REG 255 
#define IF_LABEL 0
#define LOOP_LABEL 1

extern FILE *outfile; // main.c
extern int count; // main.c
extern struct SymbolTable *sym_table;
extern int expr_label;

static char *global_buf[1000];
static int global_buf_size = 0;

int count_init(char *func_id)
{
    if (strcmp(func_id, "main") == 0) {
        return 1;
    } else {
        return 0;
    }
}

void gen_initialize()
{
    fprintf(outfile, "; output.j\n");
    fprintf(outfile, ".class public output\n");
    fprintf(outfile, ".super java/lang/Object\n");
    // For convenience
    fprintf(outfile, ".field public static _sc Ljava/util/Scanner;\n\n");
}

const char *get_global_output_type(type_type type)
{
    switch (type) {
        case DOUBLE_t:
            return "D";
            break;
        case FLOAT_t:
            return "F";
            break;
        case INT_t:
            return "I";
            break;
        case BOOL_t:
        //case BOOLEAN_t:
            return "Z";
            break;
        default :
            printf("No this type\n");
            return "";
    }
}

void gen_var_decl(struct Entry *entry)
{
    if (entry) {
        if (entry->kind == CONSTANT_t)
            return; // Don't need to generate code for constant

        if (entry->level == 0) {// global
            fprintf(outfile, ".field public static %s ", entry->name);
            fprintf(outfile, "%s\n", get_global_output_type(entry->type->type));

        } else { // local
            entry->location = count++;
            if (entry->type->type == DOUBLE_t)
                count++; // double has more byte
        }
    }
}

void gen_load_constant(type_type type, struct Value *val, char *expr_buf)
{
    switch (type) {
        case DOUBLE_t:
            sprintf(expr_buf, "ldc2_w %lf", val->d_val);
            break;
        case FLOAT_t:
            sprintf(expr_buf, "ldc %lf", val->d_val);
            break;
        case INT_t:
            sprintf(expr_buf, "ldc %d", val->i_val);
            break;
        case BOOL_t:
            sprintf(expr_buf, "iconst_%d", val->i_val);
            break;
        case STRING_t:
            sprintf(expr_buf, "ldc \"%s\"", val->string_literal);
            break;
        default:
            printf("No this type for value");
            break;
    }
}

void gen_load_value_val(struct Value *val)
{
    if (val) {
        char expr_buf[100];
        gen_load_constant(val->type->type, val, expr_buf);

        if (sym_table->cur_level == 0) {
            push_global_buf(expr_buf);
        } else {
            fprintf(outfile, "%s\n", expr_buf);
        }
        //fprintf(outfile, "%s\n", expr_buf);
    }
}

void gen_load_entry_val(struct Entry *entry)
{
    if (entry) {
        char expr_buf[100]; // just a big number for convenient
        if (entry->kind == CONSTANT_t) {
            gen_load_constant(entry->type->type, entry->attr->value, expr_buf);
        } else { // variable
            if (entry->level == 0) { // global
                sprintf(expr_buf, "getstatic output/%s %s", entry->name, get_global_output_type(entry->type->type));
            } else { // local
                switch (entry->type->type) {
                    case DOUBLE_t:
                        sprintf(expr_buf, "dload %d", entry->location);
                        break;
                    case FLOAT_t:
                        sprintf(expr_buf, "fload %d", entry->location);
                        break;
                    case INT_t:
                    case BOOL_t:
                        sprintf(expr_buf, "iload %d", entry->location);
                        break;
                    default:
                        break;
                }
            }
        }

        if (sym_table->cur_level == 0) // global
            push_global_buf(expr_buf);
        else
            fprintf(outfile, "%s\n", expr_buf);

        //fprintf(outfile, "%s\n", expr_buf);
    }
}

type_type stack_element_coercion(type_type v1_type, type_type v2_type)
{
    if (v1_type == DOUBLE_t) {
        if (v2_type == FLOAT_t) {
            fprintf(outfile, "f2d\n");
        } else if (v2_type == INT_t) {
            fprintf(outfile, "i2d\n");
        }
        return DOUBLE_t;
    } else if (v2_type == DOUBLE_t) {
        fprintf(outfile, "dstore %d\n", TEMP_STACK_REG); // v2
        if (v1_type == FLOAT_t) {
            fprintf(outfile, "f2d\n");
        } else if (v1_type == INT_t) {
            fprintf(outfile, "i2d\n");
        }
        fprintf(outfile, "dload %d\n", TEMP_STACK_REG);
        return DOUBLE_t;
    } else if (v1_type == FLOAT_t) {
        if (v2_type == INT_t) {
            fprintf(outfile, "i2f\n");
        }
        return FLOAT_t;
    } else if (v2_type == FLOAT_t) {
        if (v1_type == INT_t) {
            fprintf(outfile, "fstore %d\n", TEMP_STACK_REG); // v2
            fprintf(outfile, "i2f\n");
            fprintf(outfile, "fload %d\n", TEMP_STACK_REG);
        }
        return FLOAT_t;
    } else {
        return INT_t;
        // v1_type and v2_type == int
    }
}


void gen_arith_expr(struct Value *v1, struct Value *v2, Operator_Type op)
{
    // Only have double, float, int
    if (v1 && v2) {
        type_type v1_type = v1->type->type;
        type_type v2_type= v2->type->type;
        type_type result_type;
        char expr_buf[10];

        result_type = stack_element_coercion(v1_type, v2_type);

        switch (op) {
            case ADD:
                if (result_type == INT_t) {
                    strcpy(expr_buf, "iadd");
                } else if (result_type == FLOAT_t) {
                    strcpy(expr_buf, "fadd");
                } else if (result_type == DOUBLE_t) {
                    strcpy(expr_buf, "dadd");
                }
                break;
            case SUB:
                if (result_type == INT_t) {
                    strcpy(expr_buf, "isub");
                } else if (result_type == FLOAT_t) {
                    strcpy(expr_buf, "fsub");
                } else if (result_type == DOUBLE_t) {
                    strcpy(expr_buf, "dsub");
                }
                break;
            case MUL:
                if (result_type == INT_t) {
                    strcpy(expr_buf, "imul");
                } else if (result_type == FLOAT_t) {
                    strcpy(expr_buf, "fmul");
                } else if (result_type == DOUBLE_t) {
                    strcpy(expr_buf, "dmul");
                }
                break;
            case DIV:
                if (result_type == INT_t) {
                    strcpy(expr_buf, "idiv");
                } else if (result_type == FLOAT_t) {
                    strcpy(expr_buf, "fdiv");
                } else if (result_type == DOUBLE_t) {
                    strcpy(expr_buf, "ddiv");
                }
                break;
            case MOD:
                if (result_type == INT_t) {
                    strcpy(expr_buf, "irem");
                }
                break;
            default:
                printf("Error operator\n");
                break;
        }

        if (sym_table->cur_level == 0)
            push_global_buf(expr_buf);
        else
            fprintf(outfile, "%s\n", expr_buf);
        //fprintf(outfile, "%s\n", expr_buf);
    }
}

void gen_not_expr(struct Value *v)
{
    if (v) {
        char expr_buf[10];
        fprintf(outfile, "iconst_1\n");
        strcpy(expr_buf, "ixor");

        if (sym_table->cur_level == 0)
            push_global_buf(expr_buf);
        else
            fprintf(outfile, "%s\n", expr_buf);

        //fprintf(outfile, "%s\n", expr_buf);
    }
}

void gen_neg_expr(struct Value *v)
{
    // unary operand
    if (v) {
        type_type result_type = v->type->type;
        char expr_buf[10];

        switch (result_type) {
            case DOUBLE_t:
                strcpy(expr_buf, "dneg");
                break;
            case FLOAT_t:
                strcpy(expr_buf, "fneg");
                break;
            case INT_t:
                strcpy(expr_buf, "ineg");
                break;
            default:
                printf("No this type for '-'\n");
                break;
        }

        if (sym_table->cur_level == 0)
            push_global_buf(expr_buf);
        else   
            fprintf(outfile, "%s\n", expr_buf);
        //fprintf(outfile, "%s\n", expr_buf);
    }
}

void gen_logical_expr(struct Value *v1, struct Value *v2, Operator_Type op)
{
    // only has bool
    if (v1 && v2) {
        char expr_buf[10];
        switch (op) {
            case AND:
                strcpy(expr_buf, "iand");
                break;
            case OR:
                strcpy(expr_buf, "ior");
                break;
            default:
                printf("No this operator\n");
                break;
        }

        if (sym_table->cur_level == 0)
            push_global_buf(expr_buf);
        else
            fprintf(outfile, "%s\n", expr_buf);
        //fprintf(outfile, "%s\n", expr_buf);
    }
}


void gen_rel_expr(struct Value *v1, struct Value *v2, Operator_Type op)
{
    // type is double, float, int
    if (v1 && v2) {
        type_type result_type;
        type_type v1_type = v1->type->type;
        type_type v2_type = v2->type->type;
        result_type = stack_element_coercion(v1_type, v2_type);

        switch (result_type) {
            case DOUBLE_t:
                fprintf(outfile, "dcmpl\n");
                break;
            case FLOAT_t:
                fprintf(outfile, "fcmpl\n");
                break;
            case INT_t:
                fprintf(outfile, "isub\n");
                break;
            default:
                printf("No this result type\n");
                break;
        }

        int label_1 = expr_label++; // jump addr when condition is true
        int label_2 = expr_label++; // jump addr when condition is false
        switch (op) {
            case LESS:
                fprintf(outfile, "iflt Lrel_%d\n", label_1);
                break;
            case LESS_EQ:
                fprintf(outfile, "ifle Lrel_%d\n", label_1);
                break;
            case GREAT:
                fprintf(outfile, "ifgt Lrel_%d\n", label_1);
                break;
            case GREAT_EQ:
                fprintf(outfile, "ifge Lrel_%d\n", label_1);
                break;
            case EQUAL:
                fprintf(outfile, "ifeq Lrel_%d\n", label_1);
                break;
            case NEQUAL:
                fprintf(outfile, "ifne Lrel_%d\n", label_1);
                break;
            default:
                printf("No this relation operator\n");
                break;
        }

        fprintf(outfile, "iconst_0\n");
        fprintf(outfile, "goto Lrel_%d\n", label_2);
        fprintf(outfile, "Lrel_%d:\niconst_1\n", label_1);
        fprintf(outfile, "Lrel_%d:\n", label_2);
    }
}

void gen_assignment(struct Entry *entry, struct Value *val)
{
    if (entry && val) {
        type_type entry_type = entry->type->type;
        type_type val_type = val->type->type;
        char temp_buf[150];

        switch (entry_type) {
            case DOUBLE_t:
                if (val_type == FLOAT_t) {
                    fprintf(outfile, "f2d\n");
                } else if (val_type == INT_t) {
                    fprintf(outfile, "i2d\n");
                }

                if (entry->level != 0) { // local
                    fprintf(outfile, "dstore %d\n", entry->location);
                } else { // global
                    if (sym_table->cur_level == 0) {
                        sprintf(temp_buf, "putstatic output/%s D\n", entry->name);
                        push_global_buf(temp_buf);
                    } else { 
                        fprintf(outfile, "putstatic output/%s D\n", entry->name);
                    }
                    //fprintf(outfile, "putstatic output/%s D\n", entry->name);
                }
                break;
            case FLOAT_t:
                if (val_type == INT_t) {
                    fprintf(outfile, "i2f\n");
                }

                if (entry->level != 0) { // local
                    fprintf(outfile, "fstore %d\n", entry->location);
                } else { // global
                    if (sym_table->cur_level == 0) {
                        sprintf(temp_buf, "putstatic output/%s F\n", entry->name);
                        push_global_buf(temp_buf);
                    } else {
                        fprintf(outfile, "putstatic output/%s F\n", entry->name);
                    }
                    //fprintf(outfile, "putstatic output/%s F\n", entry->name);
                }
                break;
            case INT_t:
                if (entry->level != 0) { // local
                    fprintf(outfile, "istore %d\n", entry->location);
                } else { // global
                    if (sym_table->cur_level == 0) {
                        sprintf(temp_buf, "putstatic output/%s I\n", entry->name);
                        push_global_buf(temp_buf);
                    } else {
                        fprintf(outfile, "putstatic output/%s I\n", entry->name);
                    }
                    //fprintf(outfile, "putstatic output/%s I\n", entry->name);
                }
                break;
            case BOOL_t:
                if (entry->level != 0) { // local
                    fprintf(outfile, "istore %d\n", entry->location);
                } else { // global
                    if (sym_table->cur_level == 0) {
                        sprintf(temp_buf, "putstatic output/%s Z\n", entry->name);
                        push_global_buf(temp_buf);
                    } else {
                        fprintf(outfile, "putstatic output/%s Z\n", entry->name);
                    }
                    //fprintf(outfile, "putstatic output/%s Z\n", entry->name);
                }
                break;
            default:
                printf("No this type for assignment\n");
                break;
        }
    }
}

void gen_print_statement_start()
{
    fprintf(outfile, "getstatic java/lang/System/out Ljava/io/PrintStream;\n");
}

void gen_print_statement(struct Value *val)
{
    if (val) {
        switch (val->type->type) {
            case DOUBLE_t:
                fprintf(outfile, "invokevirtual java/io/PrintStream/print(D)V\n");
                break;
            case FLOAT_t:
                fprintf(outfile, "invokevirtual java/io/PrintStream/print(F)V\n");
                break;
            case INT_t:
                fprintf(outfile, "invokevirtual java/io/PrintStream/print(I)V\n");
                break;
            case STRING_t:
                fprintf(outfile, "invokevirtual java/io/PrintStream/print(Ljava/lang/String;)V\n");
                break;
            case BOOL_t:
                fprintf(outfile, "invokevirtual java/io/PrintStream/print(Z)V\n");
                break;
            default:
                printf("Can't print this type\n");
                break;
        }
    }
}

void gen_read_statement(struct Entry *entry)
{
    if (entry) {
        switch (entry->type->type) {
            case DOUBLE_t:
                fprintf(outfile, "getstatic output/_sc Ljava/util/Scanner;\n");
                fprintf(outfile, "invokevirtual java/util/Scanner/nextDouble()D\n");
                if (entry->level == 0) {
                    fprintf(outfile, "putstatic output/%s D\n", entry->name);
                } else {
                    fprintf(outfile, "dstore %d\n", entry->location);
                }
                break;
            case FLOAT_t:
                fprintf(outfile, "getstatic output/_sc Ljava/util/Scanner;\n");
                fprintf(outfile, "invokevirtual java/util/Scanner/nextFloat()F\n");
                if (entry->level == 0) {
                    fprintf(outfile, "putstatic output/%s F\n", entry->name);
                } else {
                    fprintf(outfile, "fstore %d\n", entry->location);
                }
                break;
            case INT_t:
                fprintf(outfile, "getstatic output/_sc Ljava/util/Scanner;\n");
                fprintf(outfile, "invokevirtual java/util/Scanner/nextInt()I\n");
                if (entry->level == 0) {
                    fprintf(outfile, "putstatic output/%s I\n", entry->name);
                } else {
                    fprintf(outfile, "istore %d\n", entry->location);
                }
                break;
            case BOOL_t:
                fprintf(outfile, "getstatic output/_sc Ljava/util/Scanner;\n");
                fprintf(outfile, "invokevirtual java/util/Scanner/nextBoolean()Z\n");
                if (entry->level == 0) {
                    fprintf(outfile, "putstatic output/%s Z\n", entry->name);
                } else {
                    fprintf(outfile, "istore %d\n", entry->location);
                }
                break;
            default:
                printf("No this type for read statement\n");
                break;
        }

    }
}
void gen_if_start() 
{
    int new_label = expr_label++;
    push_to_stack(new_label, IF_LABEL); // false 
    fprintf(outfile, "ifeq Lelse_%d\n", new_label);
}

void gen_else_statement()
{
    int label = get_stack_top(IF_LABEL);
    fprintf(outfile, "goto Lifexit_%d\n", label);
    fprintf(outfile, "Lelse_%d:\n", label);
}

void gen_if_exit()
{
    int label = pop_stack();
    fprintf(outfile, "Lifexit_%d:\n", label);
}

void gen_for_start()
{
    int new_label = expr_label++;
    push_to_stack(new_label, LOOP_LABEL);
    fprintf(outfile, "Lcheck_%d:\n", new_label);
}

void gen_for_condition_jump()
{
    int label = get_stack_top(LOOP_LABEL);
    fprintf(outfile, "ifne Lbody_%d\n", label); // condition is true
    fprintf(outfile, "goto Lexit_%d\n", label); // condition is false
    fprintf(outfile, "Lbegin_%d:\n", label); // increment i
}

void gen_for_body()
{
    int label = get_stack_top(LOOP_LABEL);
    fprintf(outfile, "goto Lcheck_%d\n", label); // recheck condition
    fprintf(outfile, "Lbody_%d:\n", label);
}

void gen_for_exit_incr()
{
    int label = pop_stack();
    fprintf(outfile, "goto Lbegin_%d\n", label);
    fprintf(outfile, "Lexit_%d:\n", label);
}

void gen_while_start()
{
    int new_label = expr_label++;
    push_to_stack(new_label, LOOP_LABEL);
    fprintf(outfile, "Lbegin_%d:\n", new_label); // check condition
    //fprintf(outfile, "Linc_%d:\n", new_label);
}

void gen_while_condition_jump()
{
    int label = get_stack_top(LOOP_LABEL);
    /*fprintf(outfile, "ifne Lbody_%d\n", label);
    fprintf(outfile, "goto Lexit_%d\n", label);
    fprintf(outfile, "Lbody_%d:\n", label);*/
    fprintf(outfile, "ifeq Lexit_%d\n", label);
}

void gen_while_exit()
{
    int label = pop_stack();
    fprintf(outfile, "goto Lbegin_%d\n", label);
    fprintf(outfile, "Lexit_%d:\n", label);
}

void gen_do_while_start()
{
    int new_label = expr_label++;
    push_to_stack(new_label, LOOP_LABEL);
    fprintf(outfile, "Lbody_%d:\n", new_label);
}

void gen_do_while_condition()
{
    int label = get_stack_top(LOOP_LABEL);
    fprintf(outfile, "Lbegin_%d:\n", label); // check condition
}

void gen_do_while_exit()
{
    int label = pop_stack();
    fprintf(outfile, "ifne Lbody_%d\n", label);
    fprintf(outfile, "goto Lexit_%d\n", label);
    fprintf(outfile, "Lexit_%d:\n", label);
}

void gen_break_statement()
{
    // Can't just pop, since it may be in if-else statement
    int label = get_stack_top(LOOP_LABEL);
    fprintf(outfile, "goto Lexit_%d\n", label);
}

void gen_continue_statement()
{
    int label = get_stack_top(LOOP_LABEL);
    fprintf(outfile, "goto Lbegin_%d\n", label);
}

void gen_function(struct Entry *entry)
{
    if (entry) {
        type_type return_type = entry->type->type;

        if (strcmp(entry->name, "main") == 0) {
            fprintf(outfile, ".method public static main([Ljava/lang/String;)V\n");
            fprintf(outfile, ".limit stack 256\n");
            fprintf(outfile, ".limit locals 256\n");
            fprintf(outfile, "new java/util/Scanner\n");
            fprintf(outfile, "dup\n");
            fprintf(outfile, "getstatic java/lang/System/in Ljava/io/InputStream;\n");
            fprintf(outfile, "invokespecial java/util/Scanner/<init>(Ljava/io/InputStream;)V\n");
            fprintf(outfile, "putstatic output/_sc Ljava/util/Scanner;\n");
            write_global_buf(); // write out those that was declared in global scope 
            return;
        }

        struct Argu *argu_temp;
        char arguement_list_type[300];
        strcpy(arguement_list_type, "(");
        for (argu_temp = entry->attr->argu; argu_temp != NULL; argu_temp = argu_temp->next) {
            switch (argu_temp->type->type) {
                case DOUBLE_t:
                    strcat(arguement_list_type, "D");
                    break;
                case FLOAT_t:
                    strcat(arguement_list_type, "F");
                    break;
                case INT_t:
                    strcat(arguement_list_type, "I");
                    break;
                case BOOL_t:
                    strcat(arguement_list_type, "Z");
                    break;
                default:
                    printf("No this type in argument list\n");
                    break;
            }
        }
        strcat(arguement_list_type, ")");
        switch (return_type) {
            case DOUBLE_t:
                strcat(arguement_list_type, "D");
                break;
            case FLOAT_t:
                strcat(arguement_list_type, "F");
                break;
            case INT_t:
                strcat(arguement_list_type, "I");
                break;
            case BOOL_t:
                strcat(arguement_list_type, "Z");
                break;
            case VOID_t:
                strcat(arguement_list_type, "V");
                break;
            default:
                printf("No this return type\n");
                break;
        }

        fprintf(outfile, ".method public static %s%s\n", entry->name, arguement_list_type);
        fprintf(outfile, ".limit stack 256\n");
        fprintf(outfile, ".limit locals 256\n");
        write_global_buf(); // write out those that was declared in global scope
    }
}

void gen_end_function(struct Entry *entry)
{
    if (entry->type->type == VOID_t) {
        fprintf(outfile, "return\n");
    }
    //fprintf(outfile, "return\n");
    fprintf(outfile, ".end method\n");
}

void gen_return(struct Entry *entry, struct Value *val)
{
    if (entry && val) {
        type_type entry_type = entry->type->type;
        type_type value_type = val->type->type;

        if (strcmp(entry->name, "main") == 0) {
            fprintf(outfile, "return\n");
        } else { 
            switch (entry_type) {
                case DOUBLE_t:
                    if (value_type == FLOAT_t) {
                        fprintf(outfile, "f2d\n");
                    } else if (value_type == INT_t) {
                        fprintf(outfile, "i2d\n");
                    }
                    fprintf(outfile, "dreturn\n");
                    break;
                case FLOAT_t:
                    if (value_type == INT_t) {
                        fprintf(outfile, "i2f\n");
                    }
                    fprintf(outfile, "freturn\n");
                    break;
                case INT_t:
                    fprintf(outfile, "ireturn\n");
                    break;
                case BOOL_t:
                    fprintf(outfile, "ireturn\n");
                    break;
                    /*case VOID_t: // void statement won't have return 
                      fprintf(outfile, "return\n");
                      break;*/
                default:
                    //fprintf(outfile, "return\n");
                    printf("No this return type\n");
                    break;
            }
        }
    }
}

void gen_function_call(struct Entry *entry)
{
    if (entry) {
        struct Argu *temp_argu;
        fprintf(outfile, "invokestatic output/%s(", entry->name);
        for (temp_argu = entry->attr->argu; temp_argu != NULL; temp_argu = temp_argu->next) {
            switch (temp_argu->type->type) {
                case DOUBLE_t:
                    fprintf(outfile, "D");
                    break;
                case FLOAT_t:
                    fprintf(outfile, "F");
                    break;
                case INT_t:
                    fprintf(outfile, "I");
                    break;
                case BOOL_t:
                    fprintf(outfile, "Z");
                    break;
                default:
                    printf("No this type for argument\n");
                    break;
            }
        }
        fprintf(outfile, ")");

        switch (entry->type->type) {
            case DOUBLE_t:
                fprintf(outfile, "D");
                break;
            case FLOAT_t:
                fprintf(outfile, "F");
                break;
            case INT_t:
                fprintf(outfile, "I");
                break;
            case BOOL_t:
                fprintf(outfile, "Z");
                break;
            case VOID_t:
                fprintf(outfile, "V");
                break;
            default:
                printf("No this type for return\n");
                break;
        }
        fprintf(outfile, "\n");
    }
}

void gen_argu(struct Argu *argu, struct Value *val)
{
    if (argu && val) {
        switch (argu->type->type) {
            case DOUBLE_t:
                if (val->type->type == FLOAT_t) {
                    fprintf(outfile, "f2d\n");
                } else if (val->type->type == INT_t) {
                    fprintf(outfile, "i2d\n");
                }
            case FLOAT_t:
                if (val->type->type == INT_t) {
                    fprintf(outfile, "i2f\n");
                }
        }
    }
}


void push_global_buf(char *instr)
{
    global_buf[global_buf_size++] = strdup(instr);
}

void write_global_buf()
{
    for (int i = 0; i < global_buf_size; ++i) {
        if (global_buf[i]) {
            fprintf(outfile, "%s\n", global_buf[i]);
            free(global_buf[i]);
        }
    }
    global_buf_size = 0;
}

/*void clear_global_buf()
{
    for (int i = 0; i < global_buf_size; ++i) {
        if (global_buf[i]) {
            free(global_buf[i]);
        }
    }
    global_buf_size = 0;
}*/
