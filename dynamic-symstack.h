/**
 * Implementation of imperative language IFJ2019 compiler
 * @file dynamic-symstack.h
 * @brief Dynamic symbol stack interface
 */

#ifndef VUT_FIT_IFJ2019_DYNAMIC_SYM_STACK_H
#define VUT_FIT_IFJ2019_DYNAMIC_SYM_STACK_H

#include "scanner.h"
#include "dynamic-string.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @struct Symbol token
 *
 * Symbol_token represents one item, token, in dynamic symbol stack
 */
typedef struct symbol_token {
    token_t * token;
    struct symbol_token *next_item;
} symbol_token_t;

/**
 * @struct Dynamic symbol stack
 */
typedef struct dynamic_symbol_stack {
    struct symbol_token *top_item;
} dynamic_symbol_stack_t;

/**
 * @brief Initialises to an empty symbol stack
 *
 * @return Inicialised dynamic symbol stack or null if allocation fails
 */
dynamic_symbol_stack_t *sym_stackInit();

/**
 * @param s Dynamic symbol stack
 * @return Whether the dynamic symbol stack is empty
 */
bool sym_stackEmpty(dynamic_symbol_stack_t *s);

/**
 * @brief Pops top of the dynamic symbol stack
 *
 * @param s Dynamic symbol stack to be popped from
 * @return Token on top of the dynamic symbol stack, or if stack is empty NULL
 */
token_t * sym_stackPop(dynamic_symbol_stack_t *s);

/**
 * @brief Return the top item of the dynamic symbol stack
 *
 * @param s Dynamic symbol stack to be popped from
 * @return Token on top of the dynamic symbol stack, or if stack is empty NULL
 */
token_t * sym_stackTopItem(dynamic_symbol_stack_t *s);

/**
 * @brief Pushes a token on top of the dynamic stack
 *
 * @param s Dynamic symbol stack to be pushed to
 * @param Token to be pushed
 * @return Whether the push was successful
 */
bool sym_stackPush(dynamic_symbol_stack_t *s, token_t *t);

/**
 * @brief Frees the dynamic symbol stack
 *
 * @param s Dynamic symbol stack to be freed
 */
void sym_stackFree(dynamic_symbol_stack_t *s);

//helper function for printing stack for testing purposes
void sym_stackPrintTokenType(token_t * token);

//prints stack
void sym_stackPrint(dynamic_symbol_stack_t * stack);

//Like top but looks as deep as specified by howMuch where 0 = top
token_t * sym_stackTraverse(dynamic_symbol_stack_t * stack, int howMuch);

//inserts a token before a token on a specified depth. 0 = top
void sym_stackDeepInsert(dynamic_symbol_stack_t * stack, token_t * token, int howMuch);
#endif //VUT_FIT_IFJ2019_DYNAMIC_SYM_STACK_H
