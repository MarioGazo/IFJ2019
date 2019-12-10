/**
 * Implementation of imperative language IFJ2019 compiler
 * @file dynamic-string.h
 * @author Mario Gazo (xgazom00)
 * @brief Dynamic string interface
 */

#ifndef VUT_FIT_IFJ2019_DYNAMIC_STRING_H
#define VUT_FIT_IFJ2019_DYNAMIC_STRING_H

#include <stdbool.h>

/**
 * @struct Dynamic string structure
 */
typedef struct {
    /** String capacity */
    unsigned int capacity;
    /** Value stored in string */
    char* text;
} dynamicString_t;

/**
 * @biref Initializes to an empty string of capacity = 1
 *
 * @param string Dynamic string to be initialized
 * @return Whether the initialization was successful
 */
bool dynamicStringInit(dynamicString_t* string);

/**
 * @brief Frees the dynamic string
 *
 * @param string Dynamic string to be freed
 */
void dynamicStringFree(dynamicString_t* string);

/**
 * @brief Appends a single character to dynamic string
 *
 * @param string Dynamic string to be appended to
 * @param c Character to be appended
 * @return Whether the attachment was successful
 */
bool dynamicStringAddChar(dynamicString_t* string, int c);

/**
 * @brief Appends an entire string to dynamic string
 *
 * @param string Dynamic string to be appended to
 * @param source String to be appended
 * @return Whether the attachment was successful
 */
bool dynamicStringAddString(dynamicString_t* string, const char* source);

/**
 * @param string Dynamic string
 * @return Text
 */
char* dynamicStringGetText(dynamicString_t string);

/**
 * @param string1 First dynamic string
 * @param string2 Second dynamic string
 * @return Result of comparison of strings
 */
int dynamicStringStrCmp(dynamicString_t string1, dynamicString_t string2);

#endif //VUT_FIT_IFJ2019_DYNAMIC_STRING_H
