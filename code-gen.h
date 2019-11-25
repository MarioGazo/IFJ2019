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
static bool cg_default_value_type(TYP_datatyp typ);
//todo komentár
bool cg_fun_retval(Data_type typ);
//todo komentár
bool cg_fun_call(char *id_funkcie);
//todo komentár
bool cg_fun_retval_assign(char *ID_val_l, Data_type typ_l, Data_type navratovy_typ);
//todo komentár
bool cg_fun_param_declare(char *id_parametra, int index);
//todo komentár
bool cg_fun_before_params();
//todo komentár
bool cg_fun_convert_passed_param(Data_type z, Data_type do_, int index);
//todo komentár
bool cg_fun_pass_param(Token token, int index);
//todo komentár
bool cg_fun_return();
//todo komentár
bool cg_var_declare(char *ID_premenna);
//todo komentár
bool cg_var_default_value(char *ID_premenna, Data_type typ);
//todo komentár
static bool cg_label(char *ID_funkcie, int uni_a, int uni_b);

bool cg_while_start(int uni_a, int uni_b);
bool cg_while_end(int uni_a, int uni_b);
bool cg_if_start(int uni_a, int uni_b);
bool cg_if_else_part(int uni_a, int uni_b);
bool cg_if_end(int uni_a, int uni_b);
bool cg_print(char *hodnota, Data_type typ);

#endif //VUT_FIT_IFJ2019_CODE_GEN_H
