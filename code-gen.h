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
//todo komentár
static bool cg-define_b_i_functions;
//todo komentár
bool generate_main_scope_start();
//todo komentár
bool generate_main_scope_end();
//todo komentár
bool generate_function_start(char *id_funkcie);
//todo komentár
bool generate_function_end(char *id_funkcie);
//todo komentár
static bool gen_default_value_type(TYP_datatyp typ);
//todo komentár
bool generate_function_retval(Data_type typ);
//todo komentár
bool generate_function_call(char *id_funkcie);
//todo komentár
bool generate_function_retval_assign(char *ID_val_l, Data_type typ_l, Data_type navratovy_typ);
//todo komentár
bool generate_function_param_declare(char *id_parametra, int index);
//todo komentár
bool generate_function_before_params();
//todo komentár
bool generate_function_convert_passed_param(Data_type z, Data_type do_, int index);
//todo komentár
bool generate_function_pass_param(Token token, int index);
//todo komentár
bool generate_function_return(char *id_funkcie);
//todo komentár
bool generate_var_declare(char *ID_premenna);
//todo komentár
bool generate_var_default_value(char *ID_premenna, Data_type typ);
//todo komentár
static bool generate_label(char *ID_funkcie, int label_i, int label_d);
//todo komentár
bool generate_if_head();
//todo komentár
bool generate_if_start(char *ID_funkcie, int label_i, int label_d);
//todo komentár
bool generate_if_else_part(char *ID_funkcie, int label_i, int label_d);
//todo komentár
bool generate_if_end(char *ID_funkcie, int label_i, int label_d);
//todo komentár
bool generate_while_head(char *ID_funkcie, int label_i, int label_d);
//todo komentár
bool generate_while_start(char *ID_funkcie, int label_i, int label_d);
//todo komentár
bool generate_while_end(char *ID_funkcie, int label_i, int label_d);

#endif //VUT_FIT_IFJ2019_CODE_GEN_H
