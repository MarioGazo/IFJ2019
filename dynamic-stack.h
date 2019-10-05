// dynamic stack definition
// Created by Mário Gažo on 4.10.19.
//

#ifndef VUT_FIT_IFJ2019_DYNAMIC_STACK_H
#define VUT_FIT_IFJ2019_DYNAMIC_STACK_H

#include <stdbool.h>

typedef struct dynamic_stack {
    int* data;
    unsigned int capacity;
    int top;
} dynamic_stack_t;

bool stackInit(dynamic_stack_t* s);

bool stackEmpty(dynamic_stack_t* s);

int stackPop(dynamic_stack_t *s);

bool stackPush(dynamic_stack_t *s,int num);

void stackFree(dynamic_stack_t* s);

void printStack(dynamic_stack_t s);
#endif //VUT_FIT_IFJ2019_DYNAMIC_STACK_H
