#include "symstack_test.h"
// Testovací program pre zásobník symbolov použiteľný pri precedentnej analýze

int main(int argc, char *argv[])	{

    dynamic_symbol_stack_t *p_test_stack = NULL;
    token_t token;

    printf("--- Inicializácia ---\n");
    p_test_stack = sym_stackInit();

    printf("--- Je zásobník prázdny?  ---\n");
    if (sym_stackEmpty(p_test_stack)){
        printf("Áno\n");
    } else {
        printf("Nie\n");
    }

    token.tokenType = CommentStart; //18
    token.tokenAttribute.intValue = 54;

    printf("--- Push ---\n");
    sym_stackPush(p_test_stack, &token);

    printf("--- Obsah vrcholu zásobníka ---\n");
    printf("--- Očakávame 18 a 54 ---\n");
    printf("\t%d\t%d\n", p_test_stack->top_item->tokenType, p_test_stack->top_item->tokenAttribute.intValue);

    printf("--- Je zásobník prázdny?  ---\n");
    if (sym_stackEmpty(p_test_stack)){
        printf("Áno\n");
    } else {
        printf("Nie\n");
    }

    printf("--- Pop ---\n");
    (void)sym_stackPop(p_test_stack);
    (void)sym_stackPop(p_test_stack);

    printf("--- Je zásobník prázdny?  ---\n");
    if (sym_stackEmpty(p_test_stack)){
        printf("Áno\n");
    } else {
        printf("Nie\n");
    }

    printf("--- 6 x Push ---\n");

    token.tokenType = CommentStart; //18
    token.tokenAttribute.intValue = 54;
    sym_stackPush(p_test_stack, &token);

    token.tokenType = OctalNum; //40
    token.tokenAttribute.doubleValue = 4.787874;
    sym_stackPush(p_test_stack, &token);

    token.tokenType = StringStart; //29
    token.tokenAttribute.intValue = 400;
    sym_stackPush(p_test_stack, &token);

    token.tokenType = Assign; //16
    token.tokenAttribute.doubleValue = -0.787874;
    sym_stackPush(p_test_stack, &token);

    token.tokenType = DivideWRest; //8
    token.tokenAttribute.word.text = "ahoj jozo";
    sym_stackPush(p_test_stack, &token);

    token.tokenType = StringStart; //29
    token.tokenAttribute.word.text  = "tu to niekde musi byt";
    sym_stackPush(p_test_stack, &token);

    printf("--- Je zásobník prázdny?  ---\n");
    if (sym_stackEmpty(p_test_stack)){
        printf("Áno\n");
    } else {
        printf("Nie\n");
    }

    printf("--- Obsah vrcholu zásobníka ---\n");
    printf("--- Očakávame 29 a tu to niekde musi byt ---\n");
    printf("\t%d\t%s\n", p_test_stack->top_item->tokenType, p_test_stack->top_item->tokenAttribute.word.text);

    printf("--- 3 x Pop ---\n");

    (void)sym_stackPop(p_test_stack);
    (void)sym_stackPop(p_test_stack);
    token = sym_stackPop(p_test_stack);

    printf("--- Obsah prvku z vrcholu zásobníka po Pop ---\n");
    printf("--- Očakávame 16 a -0.787874 ---\n");
    printf("\t%d\t%f\n", token.tokenType, token.tokenAttribute.doubleValue);

    printf("--- Pop ---\n");

    (void)sym_stackPop(p_test_stack);

    printf("--- Obsah vrcholu zásobníka ---\n");
    printf("--- Očakávame 40 a 4.787874 ---\n");
    printf("\t%d\t%f\n", p_test_stack->top_item->tokenType, p_test_stack->top_item->tokenAttribute.doubleValue);

    printf("--- Vyprázdnenie a odalokovanie pamäti  ---\n");

    sym_stackFree(p_test_stack);

    return 0;
}