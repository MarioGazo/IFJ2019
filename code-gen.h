/**
 * Implementation of imperative language IFJ2019 compiler
 * @file code-gen.h
 * @author Pavol Dubovec (xdubov02), Juraj Lazur (xlazur00)
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

#define MAX_NUMBER_OF_DIGITS 45

#define ADD_INST(_inst) \
    if(!dynamicStringAddString(&code,(_inst "\n"))) return false

#define ADD_CODE(_code) \
    if(!dynamicStringAddString(&code, (_code))) return false

#define ADD_CODE_INT(_code) \
    do {                                        \
        char string[MAX_NUMBER_OF_DIGITS];      \
        sprintf(string,"%u",_code);            \
        ADD_CODE(string);                       \
    } while (0)

#define ADD_CODE_DOUBLE(_code) \
    do {                                        \
        char string[MAX_NUMBER_OF_DIGITS];      \
        sprintf(string,"float@%a",_code);            \
        ADD_CODE(string);                       \
    } while (0)

#define FUNCTION_LEN                 \
    "\n# Built-in function - len"    \
    "\nLABEL $function_len"          \
    "\nPUSHFRAME"                    \
    "\nSTRLEN GF@expr_result LF@%0"  \
    "\nPOPFRAME"                     \
    "\nRETURN"

#define FUNCTION_SUBSTR                                                        \
    "\n# Built-in function - substr"       \
    "\nLABEL $function_substr"             \
	"\nPUSHFRAME"                                \
	"\nDEFVAR LF@$bool"						 \
	"\nLT LF@$bool int@0 LF@%1"                                \
    "\nJUMPIFEQ $nones_return LF@$bool bool@false"         \
	"\nDEFVAR LF@$str_len"						 \
	"\nSTRLEN LF@$str_len LF@%0"                 \
	"\nSUB LF@$str_len LF@$str_len int@1"                    \
	"\nLT LF@$bool LF@%1 LF@$str_len"                                \
    "\nJUMPIFEQ $nones_return LF@$bool bool@false"         \
	"\nDEFVAR LF@$i"						 \
	"\nDEFVAR LF@$a"						 \
    "\nMOVE LF@$i LF@%1"    \
    "\nMOVE LF@$a LF@%2"    \
    "\nADD LF@$a LF@$i LF@$a"						 \
    "\nDEFVAR LF@$char"						 \
    "\nMOVE GF@expr_result string@"          \
    "\nLABEL $substr_return"\
    "\nLT LF@$bool LF@$i LF@$str_len"                                \
    "\nJUMPIFEQ $final_return LF@$bool bool@false"\
    "\nLT LF@$bool LF@$i LF@$a"                                \
    "\nJUMPIFEQ $final_return LF@$bool bool@false"\
    "\nGETCHAR LF@$char LF@%0 LF@$i"                                         \
    "\nCONCAT GF@expr_result GF@expr_result LF@$char"             \
    "\nADD LF@$i LF@$i int@1"                                            \
    "\nJUMP $substr_return"               \
    "\nLABEL $final_return"               \
    "\nPOPFRAME"								 \
	"\nRETURN"                                   \
    "\nLABEL $nones_return"                       \
    "\nMOVE GF@expr_result string@None"          \
	"\nPOPFRAME"								 \
	"\nRETURN"

#define FUNCTION_CHR                    \
    "\n# Built-in function - chr"       \
    "\nLABEL $function_chr"             \
    "\nPUSHFRAME"                       \
    "\nINT2CHAR GF@expr_result LF@%0"   \
    "\nPOPFRAME"                        \
    "\nRETURN"

#define FUNCTION_ORD                             \
	"\n# Built-in function - ord"				 \
	"\nLABEL $function_ord"						 \
	"\nPUSHFRAME"    \
    "\nDEFVAR LF@$bool"						 \
	"\nLT LF@$bool int@0 LF@%0"                                \
    "\nJUMPIFEQ $none_return LF@$bool bool@true"         \
	"\nDEFVAR LF@$str_len"						 \
	"\nSTRLEN LF@$str_len LF@%1"                 \
	"\nSUB LF@$str_len LF@$str_len int@1"                    \
	"\nLT LF@$bool LF@$str_len LF@%0"                                \
	"\nJUMPIFEQ $none_return LF@$bool bool@true"         \
	"\nSTRI2INT GF@expr_result LF@%0 LF@%1"		 \
    "\nPOPFRAME"								 \
	"\nRETURN"                                   \
    "\nLABEL $none_return"                       \
    "\nMOVE GF@expr_result string@None"          \
	"\nPOPFRAME"								 \
	"\nRETURN"

/**
 * @brief Sets dynamic string for output
 */
void set_code_output(dynamicString_t* output, hashTable* global);

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
bool cg_fun_param_declare(char* param, int num);

/**
 * @brief Assign value to parameter of function
 */
bool cg_fun_param_assign(unsigned int uni, token_t actualToken, bool local);
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
bool cg_var_declare(char* varName, dynamicString_t *in_string, bool local);
/**
 * @brief Label for jumping
 *
 * @param funcName Function name
 */
bool cg_label(char* funcName, char* reverse, unsigned int uni);

/**
 * @brief While loop head and label
 */
bool cg_while_start(unsigned int uni);

/**
 * @brief While loop tail and label
 */
bool cg_while_end(unsigned int uni);

/**
 * @brief If statement
 */
bool cg_if_start(unsigned int uni);

/**
 * @brief Else statement
 */
bool cg_if_else_part(unsigned int uni);

/**
 * @brief End of if & else statement
 */
bool cg_if_end(unsigned int uni);

/**
 * @brief Writes out value
 */
bool cg_print_literal(char* value, varType_t typ);

bool cg_print_id(hTabItem_t* varRecord, bool global);
/**
 * @brief Reads value
 */
bool cg_input(varType_t type);

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

bool cg_stack_push_id(char* symb);

bool cg_stack_push_gl(char* symb);

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

bool cg_rel_operation_stack(parserState_t operation);

bool cg_move(char* id_to_return, char* type);

bool cg_two_strings(unsigned int operation, unsigned int flag);

bool cg_LForGF(bool local);

#endif //VUT_FIT_IFJ2019_CODE_GEN_H
