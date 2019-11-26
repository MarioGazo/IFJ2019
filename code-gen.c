/**
 * Implementation of imperative language IFJ2019 compiler
 * @file code-gen.c
 * @author Pavol Dubovec (xdubov02)
 * @brief Code generator implementation
 */

#include "code-gen.h"
#include "dynamic-string.h"

#include <limits.h>
#include <stdio.h>

// Global code variable
dynamicString_t code;



void set_code_output(dynamicString_t* output) {
    code = *output;
}

static bool cg_define_b_i_functions()
{
    ADD_INST(FUNCTION_LEN);
    ADD_INST(FUNCTION_SUBSTR);
    ADD_INST(FUNCTION_ORD);
    ADD_INST(FUNCTION_CHR);
    return true;
}

bool cg_main_scope_start()
{
    ADD_INST(".IFJcode19");
    ADD_INST("\n# Zaciatok main");
    ADD_INST("LABEL $$main");
    ADD_INST("CREATEFRAME");
    ADD_INST("PUSHFRAME");
    return true;
}

bool cg_main_scope_end()
{
    ADD_INST("# Koniec main");
    ADD_INST("POPFRAME");
    ADD_INST("CLEARS");
    return true;
}

bool cg_fun_start(char *id_funkcie)
{
    ADD_CODE("\n# Začiatok funkcie "); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_CODE("LABEL $"); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_INST("PUSHFRAME");
    return true;
}

bool cg_fun_end(char *id_funkcie)
{
    ADD_CODE("# Koniec funkcie "); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_CODE("LABEL $"); ADD_CODE(id_funkcie); ADD_CODE("%return\n");
    ADD_INST("POPFRAME");
    ADD_INST("RETURN");
    return true;
}

static bool cg_default_value_type(varType_t typ)
{
    switch (typ)
    {
        case TypeInteger:
            ADD_CODE("int@0");
            break;

        case TypeDouble:
            ADD_CODE("float@0.0");
            break;

        case TypeString:
            ADD_CODE("string@");
            break;

        case TypeBool:
            ADD_CODE("bool@false");

        default:
            return false;
    }
    return true;
}


bool cg_fun_retval(varType_t typ)
{
    ADD_INST("DEFVAR LF@%navratova_hodnota");
    ADD_CODE("MOVE LF@%navratova_hodnota nil@nil");
    if (!cg_default_value_type(typ)) return false;
    ADD_CODE("\n");
    return true;
}

bool cg_fun_call(char *id_funkcie)
{
    ADD_CODE("CALL $"); ADD_CODE(id_funkcie); ADD_CODE("\n");

    return true;
}

bool cg_fun_retval_assign(char *ID_val_l, varType_t typ_l, varType_t navratovy_typ)
{
    if (typ_l == TypeDouble && navratovy_typ == TypeInteger)
    {
        ADD_INST("INT2FLOAT TF@%retval TF@%retval");
    }
    else if (typ_l == TypeInteger && navratovy_typ == TypeDouble)
    {
        ADD_INST("FLOAT2R2EINT TF@%retval TF@%retval");
    }
    ADD_CODE("MOVE LF@"); ADD_CODE(ID_val_l); ADD_CODE(" TF@%retval\n");
    return true;
}

bool cg_fun_param_declare(char *id_parametra, int index)
{
    ADD_CODE("DEFVAR LF@"); ADD_CODE(id_parametra); ADD_CODE("\n");
    ADD_CODE("MOVE LF@"); ADD_CODE(id_parametra); ADD_CODE(" LF@%"); ADD_CODE_INT(index); ADD_CODE("\n");

    return true;
}

bool cg_fun_before_params()
{
    ADD_INST("CREATEFRAME");
    return true;
}

bool cg_fun_return()
{
    ADD_INST("MOVE LF@%navratova_hodnota GF@%vysledok_vyrazu");
    ADD_INST("POPFRAME");
    ADD_INST("RETURN");
    return true;
}

bool cg_var_declare(char *ID_premenna)
{
    ADD_CODE("DEFVAR LF@"); ADD_CODE(ID_premenna); ADD_CODE("\n");
    return true;
}

bool cg_var_default_value(char *ID_premenna, varType_t typ)
{
    ADD_CODE("MOVE LF@ "); ADD_CODE(ID_premenna); ADD_CODE(" ");
    if (!cg_default_value_type(typ)) return false;
    ADD_CODE("\n");

    return true;
}

static bool cg_label(char *ID_funkcie, int uni_a, int uni_b)
{
    ADD_CODE("LABEL $"); ADD_CODE(ID_funkcie); ADD_CODE("%"); ADD_CODE_INT(uni_a);
    ADD_CODE("%"); ADD_CODE_INT(uni_b); ADD_CODE("\n");
    return true;
}

bool cg_while_start(int uni_a, int uni_b)
{
    ADD_INST("\n# Beginning of while");
    if (!cg_label("WHILE", uni_a, uni_b)) return false;

    ADD_CODE("JUMPIFEQ $"); ADD_CODE("WHILE"); ADD_CODE("%"); ADD_CODE_INT(uni_b);
    ADD_CODE("%"); ADD_CODE_INT(uni_a); ADD_CODE(" GF@%exp_result bool@false"); ADD_CODE("\n");
    return true;
}


bool cg_while_end(int uni_a, int uni_b)
{
    ADD_CODE("JUMP $"); ADD_CODE("WHILE"); ADD_CODE("%"); ADD_CODE_INT(uni_a);
    ADD_CODE("%"); ADD_CODE_INT(uni_b); ADD_CODE("\n");
    ADD_INST("# End of while");

    if (!cg_label("WHILE", uni_b, uni_a)) return false;
    return true;
}


bool cg_if_start(int uni_a, int uni_b)
{
    ADD_INST("\n# Begin of If Then");
    ADD_CODE("JUMPIFEQ $"); ADD_CODE("IF"); ADD_CODE("%"); ADD_CODE_INT(uni_a);
    ADD_CODE("%"); ADD_CODE_INT(uni_b); ADD_CODE(" GF@%exp_result bool@false\n");
    return true;
}


bool cg_if_else_part(int uni_a, int uni_b)
{
    ADD_CODE("JUMP $"); ADD_CODE("IF"); ADD_CODE("%"); ADD_CODE_INT(uni_b);
    ADD_CODE("%"); ADD_CODE_INT(uni_a); ADD_CODE("\n");
    ADD_INST("# Else part");
    if (!cg_label("IF", uni_a, uni_b)) return false;
    return true;
}


bool cg_if_end(int uni_a, int uni_b)
{
    ADD_INST("# End If");
    if (!cg_label("IF", uni_b, uni_a)) return false;
    return true;
}

bool cg_print(char *hodnota, varType_t typ)
{
    ADD_INST("\n# Function PRINT");
    switch (typ) {
        case TypeInteger:
            ADD_CODE("WRITE int@"); ADD_CODE(hodnota);
            return true;
        case TypeDouble:
            ADD_CODE("WRITE string@"); ADD_CODE(hodnota);
            return true;
        case TypeNone:
            ADD_CODE("WRITE string@None");
            return true;
        case TypeString:
            ADD_CODE("WRITE string@");
            for (unsigned long i = 0; i < strlen(hodnota); i++) {
                if (hodnota[i] < 32 || hodnota[i] == 35 || hodnota[i] == 92) {
                    char buffer[4];
                    sprintf(buffer,"%d",hodnota[i]);
                    ADD_CODE("\\0"); ADD_CODE(buffer);
                } else {
                    char c[1];
                    c[0] = hodnota[i];
                    ADD_CODE(c);
                }
            }
            return true;
        default:
            return false;
    }
}

bool cg_type(varType_t type) {
    switch (type) {
        case TypeInteger:
            ADD_CODE("int");        return true;
        case TypeString:
            ADD_CODE("string");     return true;
        case TypeDouble:
            ADD_CODE("float");      return true;
        case TypeBool:
            ADD_CODE("bool");       return true;
        default:
            return false;
    }
}

bool cg_input(hTabItem_t variable) {
    ADD_INST("# Reading variable");
    ADD_CODE("READ"); ADD_CODE(variable.key.text);  if (cg_type(variable.type) == false)
                                                                    return false;
    ADD_CODE("\n");
    return true;
    }

bool cg_frame(char which) {
        switch(which) {
            case 'G':
                ADD_CODE("GF@");
                return true;
            case 'L':
                ADD_CODE("LF@");
                return true;
            case 'T':
                ADD_CODE("TF@");
                return true;
            default:
                return false;
        }
    }
