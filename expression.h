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

int expression(FILE* lIn, dynamic_stack_t* lIStack, token_t* t, token_t* controlToken, int amountOfPassedTokens, int *ret_value_type, int inF);

//given a token, retruns an LL table position
int LLPos(token_t * token);

//Another one for hadling strings and concatenation
int LLSPos(token_t * token);

//gets token from input for testing purposes
token_t * getNewToken();

//returns the first terminal in the stack
token_t * terminalTop(dynamic_symbol_stack_t * stack, int * depth);

//Creates new token with the type as given. Used to create Nonterminal & shift tokens to push;
token_t * new_token(parserState_t type);

//switch to decide what to do with a given char from the LL table
int expSwitch( dynamic_symbol_stack_t * stack, token_t ** t, const int * depth,  char symbol, int *ret_value_type);

int cg_count(parserState_t operatio, unsigned int type_op_1, unsigned int type_op_2, int *ret_value_type);

bool cg_stack_p(token_t* token);

#endif //VUT_FIT_IFJ2019_EXPRESSION_H
