/**
 * Implementation of imperative language IFJ2019 translator
 * @file symtable.c
 * @author ...
 * @brief Symtable implementation
 */

#include "symtable.h"

hashTable *TInit(unsigned long size) {

    hashTable *new_table;

    // Kontrola, či je požadovaná veľkosť poľa aspoň 1
    if (size < 1) {
        return NULL;
    }

    // Alokovanie pamäti pre celú tabuľku pointerov na jedlotlivé itemy(tokeny)
    new_table = malloc( sizeof(hashTable) + (size)*sizeof(hTabItem_t *));

    // Kontrola alokácie pamäti
    if (new_table == NULL) {
        return NULL;
    }

    // Nastavíme všetky pointre v tabuľke na NULL
    for (unsigned long i = 0; i < size; i++)
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

    hTabItem_t *active_item, *next_item;

    // Prechádzame cez všetky prvky tabuľku a postupne ich vymazámave a uvoľnujeme pamäť
    for (unsigned long i = 0; i < hTab->size; i++)
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

hTabItem_t* TInsert(hashTable* hTab, hTabItem_t item) {

    // Zistíme, či existuje tabuľka, do ktorej chceme vkladať
    if (hTab == NULL) {
        fprintf(stderr, "Search error, non-valid key or table\n");
        return NULL;
    }

    // Použijeme hash funkciu a vytvoríme index
    unsigned int hash_key_index = (THashFunction(dynamicStringGetText(item.key)) % hTab->size);
    hTabItem_t *active_item;

    // Alokujem pamť pre nový prvok(token) v tabuľke
    hTabItem_t *new_table_item = malloc(sizeof(hTabItem_t));

    // Kontrola úspešnosti alokácie pamäti
    if (new_table_item == NULL) {
        return NULL;
    }

    // Pripravíme si nový item skopítovaním dát z item ako parametra funkcie
    new_table_item->key = item.key;
    new_table_item->defined = item.defined;
    new_table_item->type = item.type;
    // Kopírujeme typ dát, určených type
    switch (new_table_item->type) {
        case TypeInteger:
            new_table_item->value.intValue = item.value.intValue;
            break;
        case TypeDouble:
            new_table_item->value.doubleValue = item.value.doubleValue;
            break;
        case TypeString:
            new_table_item->value.word = item.value.word;
            break;
        case TypeBool:;
            break;
        case TypeUndefined:;
            break;
    }
    new_table_item->next = NULL;

    // Teraz vložíme item do tabuľky
    // Pripojíme pointer active_item na miesto, kam chceme umiestniť nový item
    active_item = hTab->variables[hash_key_index];

    // Ak sa na indexe už nachádza nejaký token, naviažeme ho na koniec tohto riadku
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
        if (!dynamicStringStrCmp(active_item->key,key)){
            break;
        }
        active_item = active_item->next;
    }

    // Vrátime ukazovateľ na item obsahujúci key, alebo v prípade neúspechu NULL
    return active_item;
}

void TDelete(hashTable* hTab, dynamicString_t key) {

    // Zistíme, či existuje tabuľka, z ktorej chceme vymazávať
    if (hTab == NULL || key.text == NULL) {
        fprintf(stderr, "Search error, non-valid key or table\n");
        return;
    }

    // Použijeme hash funkciu a vytvoríme index
    unsigned int hash_key_index = (THashFunction(key.text) % hTab->size);
    hTabItem_t *active_item;
    hTabItem_t *before_item;
    hTabItem_t *next_item;

    // Pripojíme pointer active_item na zodpovedajúci index
    active_item = before_item = next_item = hTab->variables[hash_key_index];

    if (!strcmp(active_item->key.text, key.text) && (active_item->next == NULL)){
        hTab->variables[hash_key_index] = NULL;
        free(active_item);
        return;
    }

    // Prechádzame celý riadok tabuľky s indexom hash_key_index a hľadáme zhodný key
    while (next_item != NULL)
    {
        // Ak nájdeme zhodu, ložíme si výskyt a pokračujeme ďalej po riadku
        if (!strcmp(next_item->key.text, key.text)){
            before_item = active_item;
            active_item = next_item;
        }
        // Posunieme sa na ďalší item v riadku
        next_item = next_item->next;
    }

    // Na koniec uvoľníme posledný známy výskyt key
    if (before_item != active_item){
        before_item->next = active_item->next;
        free(active_item);
    }
}

void TPrint(hashTable* hTab) {

    hTabItem_t *actual_item;

    // Kontrola, či tabuľka existuje
    if (hTab == NULL){
        return;
    }

    // Prechádzame všetky riadky tabuľky, pričom vypisujeme len neprázdne
    for (unsigned long i = 0; i < hTab->size; i++)
    {
        if (hTab->variables[i] != NULL) {
            actual_item = hTab->variables[i];
            // Vypisujeme všetky prvky riadku
            do {
                // Key
                printf("%lu:\t%s\t", i, dynamicStringGetText(actual_item->key));

                // Typ, podľa určenia
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

                // Je prvok efinovaný, alebo nie
                if (actual_item->defined == 0) {
                    printf("\tUndefined");
                } else {
                    printf("\tDefined\t");
                }

                // Podľa určeného typu, vypisujeme jeho hodnotu
                if (actual_item->type == TypeInteger) {
                    printf("\t%i\n", actual_item->value.intValue);
                } else if (actual_item->type == TypeDouble) {
                    printf("\t%f\n", actual_item->value.doubleValue);
                } else if (actual_item->type == TypeString) {
                    printf("\t%s\n", dynamicStringGetText(actual_item->value.word));
                } else if (actual_item->type == TypeBool) {
                    printf("\t%i\n", actual_item->value.intValue);
                } else {
                    printf("\tTypeUndefined");
                }
                // Posun na ďalší prvok v riadku
                actual_item = actual_item->next;
            } while (actual_item != NULL);
        }
    }
}

unsigned long THashFunction(const char *string) {
    // Hashovacia funkcia
    unsigned long h=0;
    const unsigned char *p;
    // Výpočet indexu riadku v tabuľke
    for ( p = (const unsigned char *) string; *p!='\0'; p++) {
        h = 65599 * h + *p;
    }
    return h;
}