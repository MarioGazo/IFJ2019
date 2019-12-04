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
#include <limits.h>

#include "symtable.h"
#include "scanner.h"
#include "expression.h"
#include "parser.h"

#define MAX_NUMBER_OF_DIGITS 40

#define ADD_INST(_inst) \
    if(!dynamicStringAddString(&code,(_inst "\n"))) return false

#define ADD_CODE(_code) \
    if(!dynamicStringAddString(&code, (_code))) return false

#define ADD_CODE_INT(_code) \
    do {                                        \
        char string[MAX_NUMBER_OF_DIGITS];      \
        sprintf(string,"int@%u",_code);            \
        ADD_CODE(string);                       \
    } while (0)

#define ADD_CODE_DOUBLE(_code) \
    do {                                        \
        char string[MAX_NUMBER_OF_DIGITS];      \
        sprintf(string,"double%f",_code);            \
        ADD_CODE(string);                       \
    } while (0)

#define FUNCTION_LEN \
    "\n# Built-in function Len"                \
    "\nLABEL $function_len"                    \
    "\nPUSHFRAME"                              \
    "\nDEFVAR LF@%navratova_hodnota"           \
    "\nSTRLEN LF@%navratova_hodnota LF@%0"     \
    "\nPOPFRAME"                               \
    "\nRETURN"

#define FUNCTION_SUBSTR \
    "\n# Built-in function SubStr"                                             \
    "\nLABEL $substr"                                                          \
    "\nPUSHFRAME"                                                              \
    "\nDEFVAR LF@%navratova_hodnota"                                           \
    "\nMOVE LF@%navratova_hodnota string@"                                     \
    "\nDEFVAR LF@dlzka_str"                                                    \
    "\nCREATEFRAME"                                                            \
    "\nDEFVAR TF@%0"                                                           \
    "\nMOVE TF@%0 LF@%0"                                                       \
    "\nCALL $length"                                                           \
    "\nMOVE LF@dlzka_str TF@%navratova_hodnota"                                \
    "\nDEFVAR LF@navratova_podmienka"                                          \
    "\nLT LF@navratova_podmienka LF@dlzka_str int@0"                           \
    "\nJUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\nEQ LF@navratova_podmienka LF@dlzka_str int@0"                           \
    "\nJUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\nLT LF@navratova_podmienka LF@%1 int@0"                                  \
    "\nJUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\nEQ LF@navratova_podmienka LF@%1 int@0"                                  \
    "\nJUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\nGT LF@navratova_podmienka LF@%1 LF@dlzka_str"                           \
    "\nJUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\nEQ LF@navratova_podmienka LF@%2 int@0"                                  \
    "\nJUMPIFEQ $substr$return LF@navratova_podmienka bool@true"               \
    "\nDEFVAR LF@maximum_n"                                                    \
    "\nMOVE LF@maximum_n LF@dlzka_str"                                         \
    "\nSUB LF@maximum_n LF@maximum_n LF@%1"                                    \
    "\nADD LF@maximum_n LF@maximum_n int@1"                                    \
    "\nDEFVAR LF@edit_n_podmienka"                                             \
    "\nLT LF@edit_n_podmienka LF@%2 int@0"                                     \
    "\nJUMPIFEQ $substr$edit_n LF@edit_n_podmienka bool@true"                  \
    "\nGT LF@edit_n_podmienka LF@%2 LF@maximum_n"                              \
    "\nJUMPIFEQ $substr$edit_n LF@edit_n_podmienka bool@true"                  \
    "\nJUMP $substr$process"                                                   \
    "\nLABEL $substr$edit_n"                                                   \
    "\nMOVE LF@%2 LF@maximum_n"                                                \
    "\nLABEL $substr$process"                                                  \
    "\nDEFVAR LF@index"                                                        \
    "\nMOVE LF@index LF@%1"                                                    \
    "\nSUB LF@index LF@index int@1"                                            \
    "\nDEFVAR LF@char"                                                         \
    "\nDEFVAR LF@procesna_podmienka"                                           \
    "\nLABEL $substr$process_loop"                                             \
    "\nGETCHAR LF@char LF@%0 LF@index"                                         \
    "\nCONCAT LF@%navratova_hodnota LF@%navratova_hodnota LF@char"             \
    "\nADD LF@index LF@index int@1"                                            \
    "\nSUB LF@%2 LF@%2 int@1"                                                  \
    "\nGT LF@procesna_podmienka LF@%2 int@0"                                   \
    "\nJUMPIFEQ $substr$process_loop LF@procesna_podmienka bool@true"          \
    "\nLABEL $substr$return"                                                   \
    "\nPOPFRAME"                                                               \
    "\nRETURN"

#define FUNCTION_CHR \
    "\n# Built-in function Chr"                                            \
    "\nLABEL $chr"                                                         \
    "\nPUSHFRAME"                                                          \
    "\nDEFVAR LF@%navratova_hodnota"                                       \
    "\nMOVE LF@%navratova_hodnota string@"                                 \
    "\nDEFVAR LF@rozsah_podm"                                              \
    "\nLT LF@rozsah_podm LF@%0 int@0"                                      \
    "\nJUMPIFEQ $chr$return LF@rozsah_podm bool@true"                      \
    "\nGT LF@rozsah_podm LF@%0 int@255"                                    \
    "\nJUMPIFEQ $chr$return LF@rozsah_podm bool@true"                      \
    "\nINT2CHAR LF@%navratova_hodnota LF@%0"                               \
    "\nLABEL $chr$return"                                                  \
    "\nPOPFRAME"                                                           \
    "\nRETURN"

#define FUNCTION_ORD \
	"\n# Built-in function ORD"						        \
	"\nLABEL $ord"								            \
	"\nPUSHFRAME"								            \
	"\nDEFVAR LF@%navratova_hodnota"					    \
	"\nMOVE LF@%navratova_hodnota int@0"					\
	"\nDEFVAR LF@podmienena_dlzka"						    \
	"\nLT LF@podmienena_dlzka LF@%1 int@1"					\
	"\nJUMPIFEQ $ord$return LF@podmienena_dlzka bool@true"  \
	"\nDEFVAR LF@dlzka_stringu"						        \
	"\nCREATEFRAME"							                \
	"\nDEFVAR TF@%0"							            \
	"\nMOVE TF@%0 LF@%0"							        \
	"\nCALL $length"							            \
	"\nMOVE LF@dlzka_stringu TF@%navratova_hodnota"			\
	"\nGT LF@podmienena_dlzka LF@%1 LF@dlzka_stringu"   	\
	"\nJUMPIFEQ $ord$return LF@podmienena_dlzka bool@true"	\
	"\nSUB LF@%1 LF@%1 int@1"						        \
	"\nSTRI2INT LF@%navratova_hodnota LF@%0 LF@%1"		    \
	"\nLABEL $ord$return"							        \
	"\nPOPFRAME"								            \
	"\nRETURN"

/**
 * @brief Sets dynamic string for output
 */
void set_code_output(dynamicString_t *output);

/**
 * @brief Print out dynamic string to file
 *
 * @return Error code
 */
int code_write_out(int errorCode);

/**
 * @brief Code header
 */
bool cg_code_header();

/**
 * @brief Built in functions definition
 */
bool cg_define_b_i_functions();

/**
 * @brief Main scope
 */
bool cg_main_scope();

/**
 * @brief Main scope end
 */
bool cg_code_footer();

/**
 * @brief Function start label
 */
bool cg_fun_start(char *id_funkcie);

/**
 * @brief Function return and local frame destruction
 */
bool cg_fun_end(char *id_funkcie);

/**
 * @brief Jump to function label with option of return
 */
bool cg_fun_call(char *id_funkcie);

bool cg_assign_expr_result(char* variable, bool local);
//todo komentár
//bool cg_fun_retval_assign(char *ID_val_l, varType_t typ_l, varType_t navratovy_typ);

/**
 * @brief Define local parameter of function
 */
bool cg_fun_param_declare(char *id_parametra);

//todo komentár
//bool cg_fun_convert_passed_param(varType_t z, varType_t do_, int index);
//todo komentár
//bool cg_fun_pass_param(token_t token, int index);

/**
 * @brief Function return and popframe
 */
bool cg_fun_return();

/**
 * @brief Variable definition
 *
 * @param ID_premenna Variable name
 * @param inFunc Whether the variable is local or global
 */
bool cg_var_declare(char* varName, bool inFunc);

/**
 * @brief Label for jumping
 *
 * @param funcName Function name
 */
bool cg_label(char* funcName, unsigned int uni_a, unsigned int uni_b);

/**
 * @brief While loop head and label
 */
bool cg_while_start(unsigned int uni_a, unsigned int uni_b);

/**
 * @brief While loop tail and label
 */
bool cg_while_end(unsigned int uni_a, unsigned int uni_b);

/**
 * @brief If statement
 */
bool cg_if_start(unsigned int uni_a, unsigned int uni_b);

/**
 * @brief Else statement
 */
bool cg_if_else_part(unsigned int uni_a, unsigned int uni_b);

/**
 * @brief End of if & else statement
 */
bool cg_if_end(unsigned int uni_a, unsigned int uni_b);

/**
 * @brief Writes out value
 */
bool cg_print_literal(char* value, varType_t typ);

bool cg_print_id(hTabItem_t* varRecord, bool global);
/**
 * @brief Reads value
 */
bool cg_input(hTabItem_t variable, bool local);

/**
 * @brief Type enum to text
 */
bool cg_type(varType_t type);

/**
 * @brief Assigns return value of built in function to variable
 */
bool cg_frame_assign_retval(hTabItem_t variable, bool local);

bool cg_math_operation(parserState_t operation, char* var, char* op1, char* op2);

bool cg_math_operation_stack(parserState_t operation);

bool cg_stack_push_id(char* symb, bool local);

bool cg_stack_push_literal(varType_t type, char* val);

bool cg_stack_push_int(unsigned int val);

bool cg_stack_push_double(double val);

bool cg_clear_stack();

bool cg_cat_literal(char* var, char* op1, char* op2);

bool cg_cat_id(char* var, bool local1, char* op1, bool local2, char* op2, bool local3);

bool cg_stack_pop_id(char* var, bool local);

bool cg_type_of_symb(char* var, char* symb);

bool cg_flag_gen(char* a_part, unsigned int number, char* b_part);

bool cg_stack_int2float();

bool cg_exit(int errorNum);

bool cg_jump(char* jump_type, char* flag_1_part, unsigned int flag_number, char* flag_2_part, char* op_1, char* op_2);

#endif //VUT_FIT_IFJ2019_CODE_GEN_H
