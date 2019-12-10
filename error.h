/**
 * Implementation of imperative language IFJ2019 compiler
 * @file error.h
 * @author Mario Gazo (xgazom00)
 * @brief Error code interface
 */

#ifndef VUT_FIT_IFJ2019_ERROR_H
#define VUT_FIT_IFJ2019_ERROR_H

/**
* @defgroup error Program errors codes
* In case there is an problem with input file, program return one of this error
* @{
*/

#define PROG_OK 0
#define LEX_ERR 1
#define SYNTAX_ERR 2
#define SEMPROG_ERR 3
#define SEMRUN_ERR 4
#define SEMPARAM_ERR 5
#define SEMELSE_ERR 6
#define DIVZERO_ERR 9
#define INTERNAL_ERR 99

/**
 * @}
 */

#endif //VUT_FIT_IFJ2019_ERROR_H
