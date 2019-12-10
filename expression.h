/**
 * Implementation of imperative language IFJ2019 compiler
 * @file expression.h
 * @author Vojtěch Golda (xgolda02), Juraj Lazur (xlazur00)
 * @brief Expression parser interface
 */

#ifndef VUT_FIT_IFJ2019_EXPRESSION_H
#define VUT_FIT_IFJ2019_EXPRESSION_H

#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include "dynamic-string.h"
#include "dynamic-symstack.h"
#include "dynamic-stack.h"
#include "scanner.h"

/**
* @defgroup expression Expression functions
* Functions for process expressions
* @{
*/

/**
 * @brief Main function for processing expressions
 *
 * @param lIn Input code file of IFJ19
 * @param lIStack Indent, dedent stack
 * @param t Actual processed token, in som cases second token of the expression
 * @param controlToken Token before actual token, in some cases is the first token of expression
 * @param amountOfPassedTokens Case of expression, how many token must be sent to the function before process start
 * @param ret_value_type Type of final result from process
 * @param inF Cares information, if the expression is in some function
 *
 * @return If the process of expression was successful 0, in case of error, number of error
 */
int expression(FILE* lIn, dynamic_stack_t* lIStack, token_t* t, token_t* controlToken, int amountOfPassedTokens, int *ret_value_type, int inF);

/**
 * @brief Function for returning position of token from LL table
 *
 * @param token Token of which position will be returned
 *
 * @return Position of token in LL table
 */
int LLPos(token_t * token);

/**
 * @brief Special function for returning position of token, which is string type, from LL table
 *
 * @param token Token of which position will be returned
 *
 * @return If the process of expression was successful 0, in case of error, number of error
 */
int LLSPos(token_t * token);

/**
 * @brief Call scanner and gets next token
 *
 * @return Next token with allocated memory
 */
token_t * getNewToken();

/**
 * @brief Returns the first terminal in the dynamic symbol stack
 *
 * @param stack Dynamic symbol stack from with will be token return
 * @param depth Position in dynamic symbol stack of token, which will be return
 *
 * @return Token from dynamic symbol stack
 */
token_t * terminalTop(dynamic_symbol_stack_t * stack, int * depth);

/**
 * @brief Creates new token of given type
 *
 * @param type Type of created token
 *
 * @return New token
 */
token_t * new_token(parserState_t type);

/**
 * @brief Function decides on given char from LL table what will be do next
 *
 * @param stack Dynamic symbol stack in which are stored parts of expression
 * @param t Actual processed token of expression
 * @param depth Position of token in dynamic symbol stack
 * @param symbol Actual char from LL table
 * @param ret_value_type Type of result of expression
 *
 * @return Command, what will be do next
 */
int expSwitch(dynamic_symbol_stack_t* stack, token_t** t, const int* depth,  char symbol, int* ret_value_type);

/**
 * @brief Function for printing instruction based on expression intro IFJcode19
 *
 * @param operation Actual operation
 * @param type_op_1 Type of first operand
 * @param type_op_2 Type of second operand
 * @param ret_value_type Type of result of expression
 *
 * @return If everything was ok 0, in case of error number of error
 */
int cg_count(parserState_t operation, unsigned int type_op_1, unsigned int type_op_2, int *ret_value_type);

/**
 * @brief Function create instruction to insert token to IFJcode19 stack
 *
 * @param token Token to be inserted
 */
bool cg_stack_p(token_t* token);

/**
 * @}
 */

#endif //VUT_FIT_IFJ2019_EXPRESSION_H
