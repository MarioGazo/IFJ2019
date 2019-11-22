/**
 * Implementation of imperative language IFJ2019 compiler
 * @file code-gen.c
 * @author Pavol Dubovec (xdubov02)
 * @brief Code generator implementation
 */

#include "code-gen.h"
#include "dynamic-string.h"
#include "scanner.h"

#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_NUMBER_OF_DIGITS UINT_MAX
#define ADD_INSTRUCTION(_inst)\
    if(!dynamicStringAddString(&code,(_inst "\n"))) return false

#define ADD_CODE\
    if(!dynamicStringAddString(&code, (_code))) return false

#define ADD_CODE_INT(_code)\
    do{
        char string[MAX_NUMBER_OF_DIGITS];                          \
        sprintf(string,"%d",_code);                                 \
        ADD_CODE(string);                                           \
    } while (1)                                                     \

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
}
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
    "\n MOVE TF@%0 LF@%0"                       \
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
"\n # Built-in function INPUTI"                  \
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
"\n # Built-in function INPUTF"                  \
    "\n LABEL $function_inputf"                  \
    "\n PUSHFRAME"                               \
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

static bool cg-define_b_i_functions
{
        ADD_INSTRUCTION(FUNCTION_INPUTS);
        ADD_INSTRUCTION(FUNCTION_INPUTI);
        ADD_INSTRUCTION(FUNCTION_INPUTF);
        ADD_INSTRUCTION(FUNCTION_PRINT);
        ADD_INSTRUCTION(FUNCTION_LEN);
        ADD_INSTRUCTION(FUNCTION_SUBSTR);
        ADD_INSTRUCTION(FUNCTION_ORD);
        ADD_INSTRUCTION(FUNCTION_CHR);
}

bool generate_main_scope_start()
{
    ADD_INSTRUCTION("\n# Zaciatok main");
    ADD_INSTRUCTION("LABEL $$main");
    ADD_INSTRUCTION("CREATEFRAME");
    ADD_INSTRUCTION("PUSHFRAME");
    return true;
}

bool generate_main_scope_end()
{
    ADD_INSTRUCTION("# Koniec main");
    ADD_INSTRUCTION("POPFRAME");
    ADD_INSTRUCTION("CLEARS");
    return true;
}

bool generate_function_start(char *id_funkcie)
{
    ADD_CODE("\n# Začiatok funkcie "); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_CODE("LABEL $"); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_INST("PUSHFRAME");
    return true;
}

bool generate_function_end(char *id_funkcie)
{
    ADD_CODE("# Koniec funkcie "); ADD_CODE(id_funkcie); ADD_CODE("\n");
    ADD_CODE("LABEL $"); ADD_CODE(id_funkcie); ADD_CODE("%return\n");
    ADD_INSTRUCTION("POPFRAME");
    ADD_INSTRUCTION("RETURN");
    return true;
}

static bool gen_default_value_type(TYP_datatyp typ)
{
    switch (typ)
    {
        case TYP_INT:
            ADD_CODE("int@0");
            break;

        case TYP_DOUBLE:
            ADD_CODE("float@0.0");
            break;

        case TYP_STRING:
            ADD_CODE("string@");
            break;

        case TYP_BOOL:
            ADD_CODE("bool@false");

        default:
            return false;
    }
    return true;
}


bool generate_function_retval(Data_type typ)
{
    ADD_INSTRUCTION("DEFVAR LF@%navratova_hodnota");
    ADD_CODE("MOVE LF@%navratova_hodnota");
    if (!gen_default_value_type(typ)) return false;
    ADD_CODE("\n");
    return true;
}

bool generate_function_call(char *id_funkcie)
{
    ADD_CODE("CALL $"); ADD_CODE(id_funkcie); ADD_CODE("\n");

    return true;
}

bool generate_function_retval_assign(char *ID_val_l, Data_type typ_l, Data_type navratovy_typ)
{
    if (typ_l == TYPE_DOUBLE && navratovy_typ == TYPE_INT)
    {
        ADD_INST("INT2FLOAT TF@%retval TF@%retval");
    }
    else if (typ_l == TYPE_INT && navratovy_typ == TYPE_DOUBLE)
    {
        ADD_INST("FLOAT2R2EINT TF@%retval TF@%retval");
    }
    ADD_CODE("MOVE LF@"); ADD_CODE(ID_val_l); ADD_CODE(" TF@%retval\n");
    return true;
}

bool generate_function_param_declare(char *id_parametra, int index)
{
    ADD_CODE("DEFVAR LF@"); ADD_CODE(id_parametra); ADD_CODE("\n");
    ADD_CODE("MOVE LF@"); ADD_CODE(id_parametra); ADD_CODE(" LF@%"); ADD_CODE_INT(index); ADD_CODE("\n");

    return true;
}

bool generate_function_before_params()
{
    ADD_INSTRUCTION("CREATEFRAME");
    return true;
}

bool generate_function_convert_passed_param(Data_type z, Data_type do_, int index)
{
    if (z == TYPE_INT && do_ == TYPE_DOUBLE)
    {
        ADD_CODE("INT2FLOAT TF@%"); ADD_CODE_INT(index); ADD_CODE(" TF@%"); ADD_CODE_INT(index); ADD_CODE("\n");
    }
    else if (z == TYPE_DOUBLE && do_ == TYPE_INT)
    {
        ADD_CODE("FLOAT2R2EINT TF@%"); ADD_CODE_INT(index); ADD_CODE(" TF@%"); ADD_CODE_INT(index); ADD_CODE("\n");
    }
    return true;
}

bool generate_function_pass_param(Token token, int index)
{
    ADD_CODE("DEFVAR TF@%"); ADD_CODE_INT(index); ADD_CODE("\n");
    ADD_CODE("MOVE TF@%"); ADD_CODE_INT(index); ADD_CODE(" ");
    if (!generate_term_value(token)) return false;
    ADD_CODE("\n");
    return true;
}

bool generate_function_return(char *id_funkcie)
{
    ADD_INSTRUCTION("MOVE LF@%navratova_hodnota GF@%vysledok_vyrazu");
    ADD_CODE("JUMP $"); ADD_CODE(function_id); ADD_CODE("%return\n");
    return true;
}

bool generate_var_declare(char *ID_premenna)
{
    ADD_CODE("DEFVAR LF@"); ADD_CODE(ID_premenna); ADD_CODE("\n");
    return true;
}

bool generate_var_default_value(char *ID_premenna, Data_type typ)
{
    ADD_CODE("MOVE LF@ "); ADD_CODE(ID_premenna); ADD_CODE(" ");
    if (!generate_default_var_value(typ)) return false;
    ADD_CODE("\n");

    return true;
}

static bool generate_label(char *ID_funkcie, int label_i, int label_d)
{
    ADD_CODE("LABEL $"); ADD_CODE(ID_funkcie); ADD_CODE("%"); ADD_CODE_INT(label_d);
    ADD_CODE("%"); ADD_CODE_INT(label_i); ADD_CODE("\n");
    return true;
}

bool generate_if_head()
{
    ADD_INST("\n# If Then");
    return true;
}


bool generate_if_start(char *ID_funkcie, int label_i, int label_d)
{
    ADD_CODE("JUMPIFEQ $"); ADD_CODE(ID_funkcie); ADD_CODE("%"); ADD_CODE_INT(label_d);
    ADD_CODE("%"); ADD_CODE_INT(label_i); ADD_CODE(" GF@%exp_result bool@false\n");
    return true;
}


bool generate_if_else_part(char *ID_funkcie, int label_i, int label_d)
{
    ADD_CODE("JUMP $"); ADD_CODE(ID_funkcie); ADD_CODE("%"); ADD_CODE_INT(label_d);
    ADD_CODE("%"); ADD_CODE_INT(label_index + 1); ADD_CODE("\n");
    ADD_INST("# Else");
    if (!generate_label(ID_funkcie, label_i, label_d)) return false;
    return true;
}


bool generate_if_end(char *ID_funkcie, int label_i, int label_d)
{
    ADD_INST("# End If");
    if (!generate_label(ID_funkcie, label_i, label_d)) return false;
    return true;
}


bool generate_while_head(char *ID_funkcie, int label_i, int label_d)
{
    ADD_INST("\n# Do While");
    if (!generate_label(function_id, label_i, label_d)) return false;
    return true;
}


bool generate_while_start(char *ID_funkcie, int label_i, int label_d)
{
    ADD_CODE("JUMPIFEQ $"); ADD_CODE(ID_funkcie); ADD_CODE("%"); ADD_CODE_INT(label_d);
    ADD_CODE("%"); ADD_CODE_INT(label_i); ADD_CODE(" GF@%exp_result bool@false"); ADD_CODE("\n");

    return true;
}


bool generate_while_end(char *ID_funkcie, int label_i, int label_d)
{
    ADD_CODE("JUMP $"); ADD_CODE(ID_funkcie); ADD_CODE("%"); ADD_CODE_INT(label_d);
    ADD_CODE("%"); ADD_CODE_INT(label_i - 1); ADD_CODE("\n");
    ADD_INST("# Loop");
    if (!generate_label(ID_funkcie, label_i, label_d)) return false;
    return true;
}
