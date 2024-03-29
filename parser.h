/**
 * Implementation of imperative language IFJ2019 compiler
 * @file parser.h
 * @author Mario Gazo (xgazom00), Juraj Lazur (xlazur00)
 * @brief Parser interface
 */

#ifndef VUT_FIT_IFJ2019_PARSER_H
#define VUT_FIT_IFJ2019_PARSER_H

#include <stdio.h>
#include "symtable.h"

/**
* @defgroup parser Parser functions
* Functions for process input program
* @{
*/

/**
 * @brief Start of syntactical analysis
 *
 * @param file Input program from STDIN
 * @return Error code
 */
int analyse(FILE* file);

/**
 * @brief Main analysing function, which process all parts of the input program.
 *
 * @return Error code
 */
int program();

/**
 * @brief Function analysing user defined functions
 *
 * @return Error code
 */
int defFunction();

/**
 * @brief Function analysing parameters of user defined functions
 *
 * @param funcRecord Parameters function record
 *
 * @return Error code
 */
int param(hTabItem_t* funcRecord);

/**
 * @brief List of commands in function body, if or else statements or cycle
 *
 * @return Error code
 */
int command();

/**
 * @brief Assigment
 *
 * @param varRecord Variable to assign to
 *
 * @return Error code
 */
int assign(hTabItem_t* varRecord);

/**
 * @brief Terminal symbols
 *
 * @return Error code
 */
int term();

 /**
  * @brief Finds and returns item in hash table
  *
  * @param name Name of searched item
  *
  * @return Pointer to item, or NULL if not found
  */
hTabItem_t* isInLocalOrGlobalhTab(dynamicString_t name);

/**
 * @}
 */

#endif //VUT_FIT_IFJ2019_PARSER_H
