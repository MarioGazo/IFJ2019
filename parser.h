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
 * @brief Function parameters
 *
 * @return Error code
 */
int params(hTabItem_t* funcRecord);

/**
 * @brief List of commands in function body, if or else statements or cycle
 *
 * @return Error code
 */
int commandList();

/**
 * @brief Command
 *
 * @return Error code
 */
int command();

/**
 * @return Error code
 */
int execWhileCycle();
int execIfStatement();
int execPrint();
int execChar();
int execOrd();
int execSubstr();
int execLen();
int execInputs();
int execInputf();
int execInputi();
int execReturn();
int execPass();

#endif //VUT_FIT_IFJ2019_PARSER_H
