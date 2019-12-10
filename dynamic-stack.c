/**
 * Implementation of imperative language IFJ2019 compiler
 * @file dynamic-stack.c
 * @author Mario Gazo (xgazom00)
 * @brief Dynamic stack implementation
 */

#include "dynamic-stack.h"
#include <stdlib.h>
#include <stdio.h>

// Inicializácia stack
bool stackInit(dynamic_stack_t* s) {
    s->data = calloc(1, sizeof(int));

    if (s->data == NULL) {
        return false;
    } else {
        s->capacity = 1;
        s->top = 0;
        return true;
    }
}

// Je zásobník prázdny?
bool stackEmpty(dynamic_stack_t s) {
    return (s.top == 0);
}

// Pop
int stackPop(dynamic_stack_t* s) {
    int result = 0;

    if (stackEmpty(*s)) {
        return -1;
    } else {
        result = s->data[s->top--];
        s->data = realloc(s->data, --s->capacity * sizeof(int));
        if (s->data == NULL) {
            return -2;
        }
        return result;
    }
}

// Push
bool stackPush(dynamic_stack_t* s, int num) {
    s->data = realloc(s->data, ++s->capacity * sizeof(int));

    if (s->data == NULL) {
        return false;
    } else {
        s->data[++s->top] = num;
        return true;
    }
}

// Uvoľnenie pamäti
void stackFree(dynamic_stack_t* s) {
    free(s->data);
}

int stackTop(dynamic_stack_t s) {
    return s.data[s.top];
}
