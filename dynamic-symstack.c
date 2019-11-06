/**
 * Implementation of imperative language IFJ2019 compiler
 * @file dynamic-symstack.c
 * @brief Dynamic symbol stack implementation
 */

#include "dynamic-symstack.h"

dynamic_symbol_stack_t *sym_stackInit() {
    // Alokácia pamäte
    dynamic_symbol_stack_t *s = calloc(1, sizeof(dynamic_symbol_stack_t));

    // Ak bola alokácia neúspešná, je vrátený NULL pointer
    if (s == NULL) {
        return NULL;
    } else {
        s->top_item = NULL;
        return s;
    }
}

bool sym_stackEmpty(dynamic_symbol_stack_t *s) {
    // Ak je zásobník prázdny, neukazuje nikam
    return (s->top_item == NULL);
}

token_t sym_stackPop(dynamic_symbol_stack_t *s) {
    token_t result_token;
    symbol_token_t *token_to_be_free;

    // V prípade, že je zásobník prázdny, vráti sa token s chybovým hlásením -3
    result_token.tokenType = -3;

    if (sym_stackEmpty(s)) {
        return result_token;

    } else {
        // Posunutie ukazovateľa na vrchole zásobníka
        result_token.tokenAttribute = s->top_item->tokenAttribute;
        result_token.tokenType = s->top_item->tokenType;

        // Kopírovanie atribútov tokenu
        token_to_be_free = s->top_item;
        s->top_item = s->top_item->next_item;

        // Uvoľnenie pamäti pre token, ktorý sme pomocou pop odstránili z vrcholu zásobníka
        free(token_to_be_free);
    }

    // Vracia sa token s atribútmi prebranými zo zásobníka
    return result_token;
}

bool sym_stackPush(dynamic_symbol_stack_t *s, token_t *t) {

    if (s == NULL) {
        return false;
    }

    // Alokácia pamäti pre nový token na zásobníku
    symbol_token_t *new_token = calloc(1, sizeof(symbol_token_t));

    if (new_token == NULL){
        return false;
    }

    // Posunutie ukazovateľa na vrchol zásobníku
    new_token->next_item = s->top_item;
    s->top_item = new_token;

    // Uloženie atribútov do nového prvku zásobníku
    new_token->tokenAttribute = t->tokenAttribute;
    new_token->tokenType = t->tokenType;

    // Ak bolo uloženie úspešné, je vrátené TRUE
    return true;
}

void sym_stackFree(dynamic_symbol_stack_t *s) {
    symbol_token_t *token_to_be_free;

    // Prechádzame cez všetky tokeny na zásobníku, uvoľnujeme ich
    while (s->top_item != NULL){
        token_to_be_free = s->top_item;
        if (s->top_item != NULL){
            s->top_item = s->top_item->next_item;
        }
        free(token_to_be_free);
    }

    // Uvoľníme ukazovateľ
    free(s);
}
