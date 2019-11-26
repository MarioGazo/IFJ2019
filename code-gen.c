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

#define MAX_NUMBER_OF_DIGITS UINT_MAX
#define ADD_INST(_inst) \
    if(!dynamicStringAddString(&code,(_inst "\n"))) return false

#define ADD_CODE(_code) \
    if(!dynamicStringAddString(&code, (_code))) return false

#define ADD_CODE_INT(_code) \
    do {                                        \
        char string[MAX_NUMBER_OF_DIGITS];      \
        sprintf(string,"%d",_code);             \
        ADD_CODE(string);                       \
    } while (0)

#define FUNCTION_LEN\
    "\n # Built-in function Len"                \
    "\n LABEL $function_len"                    \
    "\n PUSHFRAME"                              \
    "\n DEFVAR LF@%navratova_hodnota"           \
    "\n STRLEN LF@%navratova_hodnota LF@%0"     \
    "\n POPFRAME"                               \
    "\n RETURN"

#define FUNCTION_SUBSTR\
    "\n # Built-in function SubStr"                                             \
    "\n LABEL $substr"                                                          \
    "\n PUSHFRAME"                                                              \
    "\n DEFVAR LF@%navratova_hodnota"                                           \
    "\n MOVE LF@%navratova_hodnota string@"                                     \
    "\n DEFVAR LF@dlzka_str"                                                    \
    "\n CREATEFRAME"                                                            \
    "\n DEFVAR TF@%0"                                                           \
    "\n MOVE TF@%0 LF@%0"                                                       \
    "\n CALL $length"                                                           \
    "\n MOVE LF@dlzka_str TF@%navratova_hodnota"                                \
    "\n DEFVAR LF@navratova_podmienka"                                          \
    "\n LT LF@navratova_podmienka LF@dlzka_str int@0"                           \
    "\n JUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\n EQ LF@navratova_podmienka LF@dlzka_str int@0"                           \
    "\n JUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\n LT LF@navratova_podmienka LF@%1 int@0"                                  \
    "\n JUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\n EQ LF@navratova_podmienka LF@%1 int@0"                                  \
    "\n JUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\n GT LF@navratova_podmienka LF@%1 LF@dlzka_str"                           \
    "\n JUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\n EQ LF@navratova_podmienka LF@%2 int@0"                                  \
    "\n JUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\n DEFVAR LF@maximum_n"                                                    \
    "\n MOVE LF@maximum_n LF@dlzka_str"                                         \
    "\n SUB LF@maximum_n LF@maximum_n LF@%1"                                    \
    "\n ADD LF@maximum_n LF@maximum_n int@1"                                    \
    "\n DEFVAR LF@edit_n_podmienka"                                             \
    "\n LT LF@edit_n_podmienka LF@%2 int@0"                                     \
    "\n JUMPIFEQ $substr$edit_n LF@edit_n_podmienka bool@true"                  \
    "\n GT LF@edit_n_podmienka LF@%2 LF@maximum_n"                              \
    "\n JUMPIFEQ $substr$edit_n LF@edit_n_podmienka bool@true"                  \
    "\n JUMP $substr$process"                                                   \
    "\n LABEL $substr$edit_n"                                                   \
    "\n MOVE LF@%2 LF@maximum_n"                                                \
    "\n LABEL $substr$process"                                                  \
    "\n DEFVAR LF@index"                                                        \
    "\n MOVE LF@index LF@%1"                                                    \
    "\n SUB LF@index LF@index int@1"                                            \
    "\n DEFVAR LF@char"                                                         \
    "\n DEFVAR LF@procesna_podmienka"                                           \
    "\n LABEL $substr$process_loop"                                             \
    "\n GETCHAR LF@char LF@%0 LF@index"                                         \
    "\n CONCAT LF@%navratova_hodnota LF@%navratova_hodnota LF@char"             \
    "\n ADD LF@index LF@index int@1"                                            \
    "\n SUB LF@%2 LF@%2 int@1"                                                  \
    "\n GT LF@procesna_podmienka LF@%2 int@0"                                   \
    "\n JUMPIFEQ $substr$process_loop LF@procesna_podmienka bool@true"          \
    "\n LABEL $substr$return"                                                   \
    "\n POPFRAME"                                                               \
    "\n RETURN"

#define FUNCTION_CHR\
    "\n # Built-in function Chr"                                            \
    "\n LABEL $chr"                                                         \
    "\n PUSHFRAME"                                                          \
    "\n DEFVAR LF@%navratova_hodnota"                                       \
    "\n MOVE LF@%navratova_hodnota string@"                                 \
    "\n DEFVAR LF@rozsah_podm"                                              \
    "\n LT LF@rozsah_podm LF@%0 int@0"                                      \
    "\n JUMPIFEQ $chr$return LF@rozsah_podm bool@true"                      \
    "\n GT LF@rozsah_podm LF@%0 int@255"                                    \
    "\n JUMPIFEQ $chr$return LF@rozsah_podm bool@true"                      \
    "\n INT2CHAR LF@%navratova_hodnota LF@%0"                               \
    "\n LABEL $chr$return"                                                  \
    "\n POPFRAME"                                                           \
    "\n RETURN"

#define FUNCTION_ORD\
	"\n # Built-in function ORD"						\
	"\n LABEL $ord"								\
	"\n PUSHFRAME"								\
	"\n DEFVAR LF@%navratova_hodnota"					\
	"\n MOVE LF@%navratova_hodnota int@0"					\
	"\n DEFVAR LF@podmienena_dlzka"						\
	"\n LT LF@podmienena_dlzka LF@%1 int@1"					\
	"\n JUMPIFEQ $ord$return LF@podmienena_dlzka bool@true"			\
	"\n DEFVAR LF@dlzka_stringu"						\
	"\n CREATEFRAME"							\
	"\n DEFVAR TF@%0"							\
	"\n MOVE TF@%0 LF@%0"							\
	"\n CALL $length"							\
	"\n MOVE LF@dlzka_stringu TF@%navratova_hodnota"			\
	"\n GT LF@podmienena_dlzka LF@%1 LF@dlzka_stringu"			\
	"\n JUMPIFEQ $ord$return LF@podmienena_dlzka bool@true"			\
	"\n SUB LF@%1 LF@%1 int@1"						\
	"\n STRI2INT LF@%navratova_hodnota LF@%0 LF@%1"				\
	"\n LABEL $ord$return"							\
	"\n POPFRAME"								\
	"\n RETURN"

#define FUNCTION_PRINT\
    "\n # Built-in function PRINT"              \
    "\n LABEL $function_print"                  \
    "\n PUSHFRAME"                              \
    "\n DEFVAR TF@%0"                           \
    "\n MOVE TF@%0 LF@pro_input"                \
    "\n WRITE TF@%0"                            \
    "\n POPFRAME"                               \
    "\n RETURN"

#define FUNCTION_INPUTS\
    "\n # Built-in function INPUTS"              \
    "\n LABEL $function_inputs"                  \
    "\n PUSHFRAME"                               \
    "\n DEFVAR LF@%navratova_hodnota"            \
    "\n MOVE LF@%navratova_hodnota LF@%1"        \
    "\n DEFVAR LF@param2"                        \
    "\n MOVE LF@param2 LF@%2"                    \
    "\n MOVE LF@param2 string@string"            \
    "\n READ LF@navratova_hodnota LF@param2"     \
    "\n MOVE LF@%navratova_hodnota string@"      \
    "\n POPFRAME"                                \
    "\n RETURN"

#define FUNCTION_INPUTI\
    "\n # Built-in function INPUTI"              \
    "\n LABEL $function_inputi"                  \
    "\n PUSHFRAME"                               \
    "\n DEFVAR LF@%navratova_hodnota"            \
    "\n MOVE LF@%navratova_hodnota LF@%1"        \
    "\n DEFVAR LF@param2"                        \
    "\n MOVE LF@param2 LF@%2"                    \
    "\n MOVE LF@param2 int@int"                  \
    "\n READ LF@navratova_hodnota LF@param2"     \
    "\n MOVE LF@%navratova_hodnota int@"         \
    "\n POPFRAME"                                \
    "\n RETURN"

#define FUNCTION_INPUTF\
    "\n # Built-in function INPUTF"              \
    "\n LABEL $function_inputf"                  \
    "\n PUSHFRAME"                               \
    "\n DEFVAR LF@%navratova_hodnota"            \
    "\n MOVE LF@%navratova_hodnota LF@%1"        \
    "\n DEFVAR LF@param2"                        \
    "\n MOVE LF@param2 LF@%2"                    \
    "\n MOVE LF@param2 float@0x0p+0@float@0x0p+0"\
    "\n READ LF@navratova_hodnota LF@param2"     \
    "\n MOVE LF@%navratova_hodnota float@0x0p+0@"\
    "\n POPFRAME"                                \
    "\n RETURN"

void set_code_output(dynamicString_t* output) {
    code = *output;
}

static bool cg_define_b_i_functions()
{
    ADD_INST(FUNCTION_INPUTS);
    ADD_INST(FUNCTION_INPUTI);
    ADD_INST(FUNCTION_INPUTF);
    ADD_INST(FUNCTION_PRINT);
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
    ADD_CODE("MOVE LF@%navratova_hodnota");
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
    ADD_CODE("MOVE LF@ "); ADD_CODE("pro_input"); ADD_CODE(" ");
    if (cg_type(typ) != true)
        return false;
    ADD_CODE("@"); ADD_CODE(hodnota);
    cg_fun_call("function_print");
    return true;
}

bool cg_type(varType_t type) {
        switch (type) {
            case TypeInteger:
                ADD_CODE("int");
                return true;
            case TypeString:
                ADD_CODE("string");
                return true;
            case TypeDouble:
                ADD_CODE("float");
                return true;
            case TypeBool:
                ADD_CODE("bool");
                return true;
            default:
                return false;
        }
    }