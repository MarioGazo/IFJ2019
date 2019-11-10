#include "symtable_test.h"

int main(int argc, char *argv[])	{

    hashTable *p_test_table = NULL;
    unsigned long size;
    hTabItem_t test_item;

    // Veľkosť tabuľky je náhodná, ideálne číslo vyberieme potom
    size = 1;

    printf("---\tTestovanie hash tabulky\t---\n");
    printf("---\tInicializacia\t---\n");

    p_test_table = TInit(size);
    dynamicString_t key, key2, key3, key4, searched_key, value_key;

    // Inicializácia dynamicString
    dynamicStringInit(&key);
    dynamicStringInit(&key2);
    dynamicStringInit(&key3);
    dynamicStringInit(&key4);
    dynamicStringInit(&searched_key);
    dynamicStringInit(&value_key);

    // Uloženie konkrétnych hodnôt dynamic string
    dynamicStringAddString(&key,"ahoj zuzi");
    dynamicStringAddString(&key2,"janko zahradka");
    dynamicStringAddString(&key3,"mortimer");
    dynamicStringAddString(&key4,"FUNKCIA T1");
    dynamicStringAddString(&searched_key,"Mortimer");
    dynamicStringAddString(&value_key,"Toto je alokovana hodnota valueString");

    printf("---\tVkladanie itemov do tabulky\t---\n");

    // Vkladanie jednotlivých item, niektoré sú s rovakými kľúčmi, aby sa otestovala funkčnosť
    // kedy na jednom riadku tabuľky je viacero prvkov
    test_item.key = key;
    test_item.next = NULL;
    test_item.value.intValue = 80;
    test_item.defined = false;
    test_item.type = TypeInteger;
    TInsert(p_test_table,test_item);

    test_item.key = key2;
    test_item.next = NULL;
    test_item.value.doubleValue = 744.855;
    test_item.defined = false;
    test_item.type = TypeDouble;
    TInsert(p_test_table,test_item);

    test_item.next = NULL;
    test_item.value.doubleValue = 1233.852;
    test_item.defined = false;
    test_item.type = TypeDouble;
    TInsert(p_test_table,test_item);

    test_item.key = key3;
    test_item.next = NULL;
    test_item.value.intValue = 12;
    test_item.defined = false;
    test_item.type = TypeInteger;
    TInsert(p_test_table,test_item);

    test_item.next = NULL;
    test_item.value.intValue = 44;
    test_item.defined = false;
    test_item.type = TypeInteger;
    TInsert(p_test_table,test_item);

    test_item.next = NULL;
    test_item.value.intValue = 7411;
    test_item.defined = false;
    test_item.type = TypeInteger;
    TInsert(p_test_table,test_item);

    test_item.next = NULL;
    test_item.value.doubleValue = -0.89864641523;
    test_item.defined = false;
    test_item.type = TypeDouble;
    TInsert(p_test_table,test_item);

    test_item.key = key4;
    test_item.next = NULL;
    test_item.defined = false;
    test_item.type = TypeFunction;
    TInsert(p_test_table,test_item);

    // Tu vkladáme do itemu, do jeho value dynamicString
    test_item.next = NULL;
    test_item.value.word = value_key;
    test_item.defined = true;
    test_item.type = TypeString;
    TInsert(p_test_table,test_item);

    printf("---\tKontrolny vypis celej tabulky\t---\n");

    printf("---\n");
    TPrint(p_test_table);
    printf("---\n");

    printf("---\tVyhladavanie key \'%s\'\t---\n", dynamicStringGetText(key3));

    // Vyhľadávanie v tabuľke pomocou key
    if (TSearch(p_test_table, key3) != NULL){
        printf("key is in the table\n");
    } else {
        printf("key is not in the table\n");
    }

    printf("---\tVyhladavanie key \'%s\'\t---\n", dynamicStringGetText(searched_key));

    if (TSearch(p_test_table, searched_key) != NULL){
        printf("key is in the table\n");
    } else {
        printf("key is not in the table\n");
    }

    dynamicStringAddString(&searched_key,"morti");

    printf("---\tVyhladavanie key \'%s\'\t---\n", dynamicStringGetText(searched_key));

    if (TSearch(p_test_table, searched_key) != NULL){
        printf("key is in the table\n");
    } else {
        printf("key is not in the table\n");
    }

    printf("---\tVymazanie posledneho vyskytu key \'%s\'\t---\n", dynamicStringGetText(key3));

    // Vymazanie itemu z tabuľky
    TDelete(p_test_table, key3);

    printf("---\tKontrolny vypis celej tabulky\t---\n");

    printf("---\n");
    TPrint(p_test_table);
    printf("---\n");

    // Viac-násobné vymazanie prvkov z tabuľky
    printf("---\t3 x vymazanie posledneho vyskytu key \'%s\'\t---\n", dynamicStringGetText(key3));
    TDelete(p_test_table, key3);
    TDelete(p_test_table, key3);
    TDelete(p_test_table, key3);

    printf("---\tKontrolny vypis celej tabulky\t---\n");

    printf("---\n");
    TPrint(p_test_table);
    printf("---\n");

    printf("---\tVyhladavanie key \'%s\'\t---\n", dynamicStringGetText(key3));

    if (TSearch(p_test_table,searched_key) != NULL){
        printf("key is in the table\n");
    } else {
        printf("key is not in the table\n");
    }

    printf("---\tOpetovne vlozenie key \'%s\'\t---\n", dynamicStringGetText(key3));

    // Opätovné vloženie prvku do tabuľky
    test_item.key = key3;
    test_item.next = NULL;
    test_item.value.doubleValue = -0.4;
    test_item.defined = true;
    test_item.type = TypeDouble;
    TInsert(p_test_table, test_item);

    printf("---\tKontrolny vypis celej tabulky\t---\n");

    printf("---\n");
    TPrint(p_test_table);
    printf("---\n");

    printf("---\tDealokacia pamate a ukoncenie\t---\n");

    // Upratovanie v pamäti
    dynamicStringFree(&key);
    dynamicStringFree(&key2);
    dynamicStringFree(&key3);
    dynamicStringFree(&key4);
    dynamicStringFree(&searched_key);
    dynamicStringFree(&value_key);
    TFree(p_test_table);
    return 0;
}