/**
 * Implementation of imperative language IFJ2019 compiler
 * @file dynamic-symstack.h
 * @author Juraj Lazur (xlazur00)
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
    /** Pointer to token */
    token_t * token;
    /** Pointer to next token */
    struct symbol_token *next_item;
} symbol_token_t;

/**
 * @struct Dynamic symbol stack
 */
typedef struct dynamic_symbol_stack {
    /** Pointer on first item */
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
 *
 * @return Whether the dynamic symbol stack is empty
 */
bool sym_stackEmpty(dynamic_symbol_stack_t *s);

/**
 * @brief Takes top token  of the dynamic symbol stack
 *
 * @param s Dynamic symbol stack to be taken from
 *
 * @return Token on top of the dynamic symbol stack, or if stack is empty NULL
 */
token_t * sym_stackPop(dynamic_symbol_stack_t *s);

/**
 * @brief Return the top item of the dynamic symbol stack
 *
 * @param s Dynamic symbol stack to be taken from
 *
 * @return Token on top of the dynamic symbol stack, or if stack is empty NULL
 */
token_t * sym_stackTopItem(dynamic_symbol_stack_t *s);

/**
 * @brief Stores a token on top of the dynamic stack
 *
 * @param s Dynamic symbol stack to be stored token to
 * @param Token to be pushed
 *
 * @return Whether the store was successful
 */
bool sym_stackPush(dynamic_symbol_stack_t *s, token_t *t);

/**
 * @brief Frees the dynamic symbol stack
 *
 * @param s Dynamic symbol stack to be freed
 */
void sym_stackFree(dynamic_symbol_stack_t *s);

/**
 * @brief Print one token of the dynamic symbol stack
 *
 * @param token Dynamic symbol stack token to be printed on stdout
 */
void sym_stackPrintTokenType(token_t * token);

/**
 * @brief Print all tokens from the dynamic symbol stack
 *
 * @param stack Dynamic symbol stack to be printed
 */
void sym_stackPrint(dynamic_symbol_stack_t * stack);

/**
 * @brief Return the token on specific position in the dynamic symbol stack
 *
 * @param stack Dynamic symbol stack to be popped from
 *
 * @return Token on token on specific position of the dynamic symbol stack, or if stack is empty NULL
 */
token_t * sym_stackTraverse(dynamic_symbol_stack_t * stack, int howMuch);

/**
 * @brief Stores a token on specific position of the dynamic symbol stack
 *
 * @param stack Dynamic symbol stack to be stored to
 * @param token Token to be stored
 * @param howMuch Position on which be token stored in dynamic symbol stack
 *
 * @return Whether the store was successful
 */
void sym_stackDeepInsert(dynamic_symbol_stack_t * stack, token_t * token, int howMuch);

#endif //VUT_FIT_IFJ2019_DYNAMIC_SYM_STACK_H
