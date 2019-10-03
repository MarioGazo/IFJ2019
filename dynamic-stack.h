// dynamic stack definition
// Created by Mário Gažo on 4.10.19.
//

#ifndef VUT_FIT_IFJ2019_DYNAMIC_STACK_H
#define VUT_FIT_IFJ2019_DYNAMIC_STACK_H

#include <stdbool.h>

typedef struct {
    int* data;
    unsigned int capacity;
    int top;
} stack_t;

bool stackInit(stack_t* s);

bool stackEmpty(stack_t* s);

int stackPop(stack_t *s);

bool stackPush(stack_t *s,int num);

void stackFree(stack_t* s);
#endif //VUT_FIT_IFJ2019_DYNAMIC_STACK_H
