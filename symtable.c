/**
 * Implementation of imperative language IFJ2019 translator
 * @file symtable.c
 * @author ...
 * @brief Symtable implementation
 */

#include "symtable.h"

hashTable *TInit(unsigned long size) {

    unsigned int i;

    hashTable *new_table;

    // Kontrola, či je požadovaná veľkosť poľa aspoň 1
    if (size < 1){
        new_table = NULL;
        return NULL;
    }

    // Alokovanie pamäti pre celú tabuľku pointerov na jedlotlivé itemy(tokeny)
    new_table = malloc( sizeof(hashTable) + (size)*sizeof(hTabItem_t *));

    // Kontrola alokácie pamäti
    if (new_table == NULL){
        new_table = NULL;
        return NULL;
    }

    // Nastavíme všetky pointre v tabuľke na NULL
    for (i = 0; i < size; i++)
    {
        new_table->variables[i] = NULL;
    }

    // Uložíme si veľkosť tabuľky
    new_table->size = size;

    // V prípade úspešnej inicializácie vraciame hodnotu true
    return new_table;
}

void TFree(hashTable* hTab) {

    // Kontrola platnosti ukazovateľa
    if (hTab == NULL){
        return;
    }

    unsigned int i;
    hTabItem_t *active_item, *next_item;

    // Prechádzame cez všetky prvky tabuľku a postupne ich vymazámave a uvoľnujeme pamäť
    for (i = 0; i < hTab->size; i++)
    {
        // Ak na zvolenom indexe nič nie je, prejdeme na ďalši index tabuľky
        if (hTab->variables[i] == NULL)
            continue;

        // Nastavíme si ukazovateľ na prvý token v riadku tabuľky určenom indexom i
        active_item = hTab->variables[i];

        // Prechádzame všetky tokeny v riadku tabuľky i a postupne ich uvoľnujeme
        do {
            next_item = active_item->next;
            free(active_item);
            active_item = next_item;
        } while (next_item != NULL);
        hTab->variables[i] = NULL;
    }

    // Nakoniec uvoľníme pamäť pre tabuľku
    free(hTab);
}

hTabItem_t* TInsert(hashTable* hTab, hTabItem_t *item) {

    // Zistíme, či existuje tabuľka, do ktorej chceme vkladať a key, ktorý chceme použiť
    if (hTab == NULL || item == NULL) {
        fprintf(stderr, "Search error, non-valid key or table\n");
        return NULL;
    }

    // Použijeme hash funkciu a vytvoríme index
    unsigned int hash_key_index = (THashFunction(item->key.text) % hTab->size);
    hTabItem_t *active_item, *next_item;

    // Alokujem pamť pre nový prvok(token) v tabuľke
    hTabItem_t *new_table_item = malloc(sizeof(hTabItem_t));

    // Kontrola úspešnosti alokácie pamäti
    if (new_table_item == NULL) {
        return NULL;
    }

    // Pripravíme si nový item skopítovaním dát z item ako parametra funkcie
    new_table_item->key.text = item->key.text;
    new_table_item->key.capacity = item->key.capacity;
    new_table_item->defined = item->defined;
    new_table_item->type = item->type;
    new_table_item->value.intValue = item->value.intValue;
    new_table_item->value.doubleValue = item->value.doubleValue;
    new_table_item->value.word.text = item->value.word.text;
    new_table_item->value.word.capacity = item->value.word.capacity;
    new_table_item->next = NULL;

    // Teraz vložíme item do tabuľky
    // Pripojíme pointer active_item na miesto, kam chceme umiestniť nový item
    active_item = hTab->variables[hash_key_index];

    // Ak sa na indexe už nachádza nejaký token, naviažeme ho na koniec tohot riadku
    if (active_item != NULL){
        while ( active_item->next != NULL) {
            active_item = active_item->next;
        }
        // Umiestnenie itemu do tabuľky
        active_item->next = new_table_item;
    // Ak je vlkadaný item prvý na indexe, naviažeme ho priamo na tabuľku
    } else {
        // Umiestnenie itemu do tabuľky
        hTab->variables[hash_key_index] = new_table_item;
    }

    // Vrátime ukazovateľ na nový item v tabuľke
    return new_table_item;
}

hTabItem_t* TSearch(hashTable* hTab, dynamicString_t key) {

    // Zistíme, či existuje tabuľka, do ktorej chceme vkladať a key, ktorý chceme použiť
    if (hTab == NULL || key.text == NULL) {
        fprintf(stderr, "Search error, non-valid key or table\n");
        return NULL;
    }

    // Použijeme hash funkciu a vytvoríme index
    unsigned int hash_key_index = (THashFunction(key.text) % hTab->size);
    hTabItem_t *active_item;

    // Pripojíme pointer active_item na zodpovedajúci index
    active_item = hTab->variables[hash_key_index];

    // Prejdeme celý riadok v tabuľke s indexom hTabItem_t *active_item a porovnávame key s uloženými items
    while (active_item != NULL)
    {
        if (!strcmp(active_item->key.text, key.text)){
            break;
        }
        active_item = active_item->next;
    }

    // Vrátime ukazovateľ na item obsahujúci key, alebo v prípade neúspechu NULL
    return active_item;
}

void TDelete(hashTable* hTab, dynamicString_t key) {

    // Zistíme, či existuje tabuľka, do ktorej chceme vkladať a key, ktorý chceme použiť
    if (hTab == NULL || key.text == NULL) {
        fprintf(stderr, "Search error, non-valid key or table\n");
        return;
    }

    // Použijeme hash funkciu a vytvoríme index
    unsigned int hash_key_index = (THashFunction(key.text) % hTab->size);
    hTabItem_t *active_item;
    hTabItem_t *before_item;

    // Pripojíme pointer active_item na zodpovedajúci index
    active_item = hTab->variables[hash_key_index];

    // Ak sa na indexe tabuľky nachádza len jeden item, uvoľnímeho a index nastavíme na NULL
    if (strcmp(active_item->key.text, key.text)){
        hTab->variables[hash_key_index] = NULL;
        free(active_item->key.text);
        free(active_item);
        return;
    }

    // Prechádzame celý riadok tabuľky s indexom hash_key_index a hľadáme zhodný key
    while (active_item != NULL)
    {
        before_item = active_item;
        active_item = active_item->next;

        if (strcmp(active_item->key.text, key.text)){
            // Nášli sme item so zhodným key
            // Nastavíme pointre, aby sa zachovala kontinuita v riadku tabuľky
            // Ak odstraňujeme posledý item v tabuľke, predcházajúci bude ukazovať na NULL
            if (active_item->next == NULL){
                before_item->next = NULL;
            // Ak za odstraňovaným itemom je ďalší item, prepojíme ich
            } else {
                before_item->next = active_item->next;
            }

            // Uvoľníme pamäť pre item
            free(active_item->key.text);
            free(active_item);
            break;
        }
    }
}

void TPrint(hashTable* hTab) {

    unsigned int i;
    hTabItem_t *actual_item;

    // Kontrola, či tabuľka existuje
    if (hTab == NULL){
        return;
    }

    // Nastavíme všetky pointre v tabuľke na NULL
    for (i = 0; i < hTab->size; i++)
    {
        if (hTab->variables[i] != NULL) {
            actual_item = hTab->variables[i];
            do {
                printf("%u:\t%s\t", i, actual_item->key.text);

                if (actual_item->type == TypeInteger) {
                    printf("\tTypeInteger");
                } else if (actual_item->type == TypeDouble) {
                    printf("\tTypeDouble");
                } else if (actual_item->type == TypeString) {
                    printf("\tTypeString");
                } else if (actual_item->type == TypeBool) {
                    printf("\tTypeBool");
                } else {
                    printf("\tTypeUndefined");
                }

                if (actual_item->defined == 0) {
                    printf("\tUndefined");
                } else {
                    printf("\tDefined");
                }

                if (actual_item->type == TypeInteger) {
                    printf("\t%i\n", actual_item->value.intValue);
                } else if (actual_item->type == TypeDouble) {
                    printf("\t%f\n", actual_item->value.doubleValue);
                } else if (actual_item->type == TypeString) {
                    printf("\t%s\n", actual_item->value.word.text);
                } else if (actual_item->type == TypeBool) {
                    printf("\t%i\n", actual_item->value.intValue);
                } else {
                    printf("\tTypeUndefined");
                }
                actual_item = actual_item->next;
            } while (actual_item != NULL);
        }
    }
}

unsigned long THashFunction(char *string) {
    unsigned long h=0;
    const unsigned char *p;
    for ( p = (const unsigned char *) string; *p!='\0'; p++) {
        h = 65599 * h + *p;
    }
    return h;
}