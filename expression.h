//
// Created by Mário Gažo on 8.10.19.
//

#ifndef VUT_FIT_IFJ2019_EXPRESSION_H
#define VUT_FIT_IFJ2019_EXPRESSION_H

#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include "dynamic-string.h"
#include "dynamic-symstack.h"
#include "scanner.h"

int expression();


//given a token, retruns an LL table position
int LLPos(token_t * token);

//gets token from input for testing purposes
token_t * getNewToken();

//returns the first terminal in the stack
token_t * terminalTop(dynamic_symbol_stack_t * stack);


//Creates new token with the type as given. Used to create Nonterminal & shift tokens to push;
token_t * new_token(tokenType type);

#endif //VUT_FIT_IFJ2019_EXPRESSION_H
