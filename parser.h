/**
 * Implementation of imperative language IFJ2019 compiler
 * @file parser.h
 * @author Mario Gazo (xgazom00)
 * @brief Parser interface
 */

#ifndef VUT_FIT_IFJ2019_PARSER_H
#define VUT_FIT_IFJ2019_PARSER_H

#include <stdio.h>
#include "symtable.h"

/**
 * @brief Start of syntactical analysis
 *
 * @param file Input file
 * @return Error code
 */
int analyse(FILE* file);

/**
 * @brief Program body
 *
 * @return Error code
 */
int program();

/**
 * @brief Function definition
 *
 * @return Error code
 */
int defFunction();

/**
 * @brief Function parameter
 *
 * @return Error code
 */
int param(hTabItem_t* funcRecord);

/**
 * @brief List of commands in function body, if or else statements or cycle
 *
 * @return Error code
 */
int commandList();

/**
 * @brief Terminal symbols
 *
 * @return Error code
 */
int term();

#endif //VUT_FIT_IFJ2019_PARSER_H
