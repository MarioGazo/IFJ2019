/**
 * Implementation of imperative language IFJ2019 compiler
 * @file dynamic-symstack.c
 * @author Juraj Lazur (xlazur00)
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

token_t * sym_stackPop(dynamic_symbol_stack_t *s) {
    token_t *  token;
    symbol_token_t *tokenf;

    // V prípade, že je zásobník prázdny, vráti sa token s chybovým hlásením -3


    if (sym_stackEmpty(s)) {
        return NULL;
    }
    tokenf = s->top_item;
    token = tokenf->token;
    s->top_item = tokenf->next_item;
    free(tokenf);

    // Vracia sa token s atribútmi prebranými zo zásobníka
    return token;
}

token_t * sym_stackTopItem(dynamic_symbol_stack_t *s) {
    if(sym_stackEmpty(s)){
      return NULL;
    }
    // Vracia sa token s atribútmi prebranými zo zásobníka
    return s->top_item->token;
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
    new_token->token = t;

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
        free(token_to_be_free->token);
        free(token_to_be_free);
    }

    // Uvoľníme ukazovateľ
    free(s);
}

// Debug funkcia pre výpis zo stack
void sym_stackPrintTokenType(token_t * token){
    if(token == NULL){
        printf("%s\n", "TOKEN IS NULL");
        return;
    }
    // Podľa druhu tokenu spravíme výpis
    switch (token->tokenType){
        case Plus:
            printf("+|");
            break;
        case Multiply:
            printf("*|");
            break;
        case LeftBracket:
            printf("(|");
            break;
        case RightBracket:
            printf(")|");
            break;
        case Identifier:
        case Integer:
            printf("%d", token->tokenAttribute.intValue);
            printf("%s", "|");
            break;
        case  EOL:
            printf("$|");
            break;
        case  Equals:
            printf("=|");
            break;
        case Nonterminal:
            printf("E|");
            break;
        case Shift:
            printf("<|");
            break;
        default:
            printf("%d", token->tokenType );
            printf("%s", "|");
    };
}

// Pomocná funkcia Debug printu zásobníka
void sym_stackPrint(dynamic_symbol_stack_t * stack){
    symbol_token_t * st = stack->top_item;
    printf("%s\n", "================");
    do {
        sym_stackPrintTokenType(st->token);
        st = st->next_item;
    } while(st!=NULL);
    printf("\n%s\n", "================");
}


token_t * sym_stackTraverse(dynamic_symbol_stack_t * stack, int howMuch){
    symbol_token_t * st = stack->top_item;
    for(int i = 0; i < howMuch; i++){
        if(st->next_item==NULL){
            return NULL;
        }
    st = st->next_item;
    }
    return st->token;
}

void sym_stackDeepInsert(dynamic_symbol_stack_t * stack, token_t * token, int howMuch){
    if(howMuch == 0){
        sym_stackPush(stack, token);
        return;
    }

    symbol_token_t * st= stack->top_item;
    for(int i = 0; i < howMuch - 1; i++){
        if(st->next_item==NULL){
            return;
        }
        st = st->next_item;
    }

    symbol_token_t *new_token = calloc(1, sizeof(symbol_token_t));

    if (new_token == NULL){
        return;
    }

    // Posunutie ukazovateľa na vrchol zásobníku
    new_token->next_item = st->next_item;
    st->next_item = new_token;

    // Uloženie atribútov do nového prvku zásobníku
    new_token->token = token;
}
