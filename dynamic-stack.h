/**
 * Implementation of imperative language IFJ2019 compiler
 * @file dynamic-stack.h
 * @author Mario Gazo (xgazom00)
 * @brief Dynamic stack interface
 */

#ifndef VUT_FIT_IFJ2019_DYNAMIC_STACK_H
#define VUT_FIT_IFJ2019_DYNAMIC_STACK_H

#include <stdbool.h>

/**
 * @struct Dynamic stack
 */
typedef struct dynamic_stack {
    int* data;
    unsigned int capacity;
    int top;
} dynamic_stack_t;

/**
 * @brief Initialises to an empty stack with top = 0
 *
 * @param s Dynamic stack to be initialized
 * @return Whether the initialization was successful
 */
bool stackInit(dynamic_stack_t* s);

/**
 * @param s Dynamic stack
 * @return Whether the dynamic stack is empty
 */
bool stackEmpty(dynamic_stack_t s);

/**
 * @brief Pops top of the dynamic stack
 *
 * @param s Dynamic stack to be popped from
 * @return Value on top of the dynamic stack
 */
int stackPop(dynamic_stack_t *s);

/**
 * @brief Pushes a value on top of the dynamic stack
 *
 * @param s Dynamic stack to be pushed to
 * @param num Value to be pushed
 * @return Whether the push was successful
 */
bool stackPush(dynamic_stack_t *s,int num);

/**
 * @brief Frees the dynamic stack
 *
 * @param s Dynamic stack to be freed
 */
void stackFree(dynamic_stack_t* s);

/**
 * @param s Dynamic stack
 * @return value on top of the dynamic stack
 */
int stackTop(dynamic_stack_t s);
#endif //VUT_FIT_IFJ2019_DYNAMIC_STACK_H
