/**
 * Implementation of imperative language IFJ2019 compiler
 * @file code-gen.h
 * @author Pavol Dubovec (xdubov02)
 * @brief Code generator header
 */
#ifndef VUT_FIT_IFJ2019_CODE_GEN_H
#define VUT_FIT_IFJ2019_CODE_GEN_H

#include <stdio.h>
#include <stdbool.h>

#include "symtable.h"
#include "scanner.h"
#include "expression.h"
#include "parser.h"

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

//todo komentár
void set_code_output(dynamicString_t *output);
//todo komentár
static bool cg_define_b_i_functions();
//todo komentár
bool cg_main_scope_start();
//todo komentár
bool cg_main_scope_end();
//todo komentár
bool cg_fun_start(char *id_funkcie);
//todo komentár
bool cg_fun_end(char *id_funkcie);
//todo komentár
static bool cg_default_value_type(varType_t typ);
//todo komentár
bool cg_fun_retval(varType_t typ);
//todo komentár
bool cg_fun_call(char *id_funkcie);
//todo komentár
bool cg_fun_retval_assign(char *ID_val_l, varType_t typ_l, varType_t navratovy_typ);
//todo komentár
bool cg_fun_param_declare(char *id_parametra, int index);
//todo komentár
bool cg_fun_before_params();
//todo komentár
bool cg_fun_convert_passed_param(varType_t z, varType_t do_, int index);
//todo komentár
bool cg_fun_pass_param(token_t token, int index);
//todo komentár
bool cg_fun_return();
//todo komentár
bool cg_var_declare(char *ID_premenna);
//todo komentár
bool cg_var_default_value(char *ID_premenna, varType_t typ);
//todo komentár
static bool cg_label(char *ID_funkcie, int uni_a, int uni_b);

bool cg_while_start(int uni_a, int uni_b);
bool cg_while_end(int uni_a, int uni_b);
bool cg_if_start(int uni_a, int uni_b);
bool cg_if_else_part(int uni_a, int uni_b);
bool cg_if_end(int uni_a, int uni_b);
bool cg_print(char *hodnota, varType_t typ);
bool cg_input(hTabItem_t variable);
bool cg_type(varType_t type);
bool cg_frame(char which);

#endif //VUT_FIT_IFJ2019_CODE_GEN_H
