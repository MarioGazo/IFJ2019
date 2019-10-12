/**
 * Implementation of imperative language IFJ2019 translator
 * @file symtable.c
 * @author ...
 * @brief Symtable implementation
 */

#include "symtable.h"

bool TInit(hashTable* hTab, unsigned long size) {
    unsigned int i;
    if (size < 1){
        hTab = NULL;
        return false;
    }

    // Memory allocation
    hashTable *n_tab = malloc( sizeof(hashTable) + (size) * sizeof(struct hTabItem_t*));

    if (n_tab == NULL){
        hTab = NULL;
        return false;
    }

    // Preparing pointers on first items on index
    for (i = 0; i < size; i++)
    {
        n_tab->variables[i] = NULL;
    }

    // Hash table size
    hTab->size = size;

    return true;
}

void TFree(hashTable* hTab) {
    if (hTab == NULL){
        return;
    }

    unsigned int i;
    struct hTabItem_t *active_item, *next_item;

    // Check every index in table and free all items on index
    for (i = 0; i < hTab->size; i++)
    {
        if (hTab->variables[i] == NULL)
            continue;

        active_item = hTab->variables[i];

        // If there is an item on index, free all items in this row
        while (active_item != NULL)
        {
            next_item = active_item->next;
            free(active_item->sting);
            free(active_item);
            active_item = next_item;
        }
        hTab->variables[i] = NULL;
    }

    // Free table
    free(hTab);
}

hTabItem_t* TInsert(hashTable* hTab, hTabItem_t item) {

    if (hTab == NULL || item == NULL) {
        fprintf(stderr, "Search error, non-valid key or table\n");
        return NULL;
    }

    // Find key index in table
    unsigned int hash_key_index = (htab_hash_function(item.key) % hTab->size);
    struct hTabItem_t *active_item;

    // Memory allocation
    struct hTabItem_t *new_table_item = malloc(sizeof(struct hTabItem_t));

    if (new_table_item == NULL) {
        return NULL;
    }

    // Copy attributes to new item in table
    (void) strcpy(new_table_item->key, item.key);
    new_table_item->defined = item.defined;
    new_table_item->type = item.type;
    new_table_item->value = item.value;
    new_table_item->next = NULL;

    // Place in table
    active_item = hTab->variables[hash_key_index];

    if (active_item == NULL){
        while (active_item != NULL)
        {
            active_item = active_item->next;
        }
        // Place as a next item, if there is key with same index
        active_item = new_table_item;
    } else {
        // Place as the first item in row
        hTab->variables[hash_key_index] = new_table_item;
    }

    return new_table_item;
}

hTabItem_t* TSearch(hashTable* hTab, dynamicString_t key) {

    if (hTab == NULL || key == NULL) {
        fprintf(stderr, "Search error, non-valid key or table\n");
        return NULL;
    }

    // Find key index in table
    unsigned int hash_key_index = (htab_hash_function(key) % hTab->size);
    struct hTabItem_t *active_item;

    active_item = hTab->variables[hash_key_index];

    // Check all item on kez index and compare keys
    while (active_item != NULL)
    {
        if (strcmp(active_item->key, key)){
            break;
        }
        active_item = active_item->next;
    }
    return active_item;
}

void TDelete(hashTable* hTab, dynamicString_t key) {

    if (hTab == NULL || key == NULL) {
        fprintf(stderr, "Search error, non-valid key or table\n");
        return NULL;
    }

    // Find key index in table
    unsigned int hash_key_index = (htab_hash_function(key) % hTab->size);
    struct hTabItem_t *active_item;
    struct hTabItem_t *before_item;

    active_item = hTab->variables[hash_key_index];

    // Find the item, that want to be deleted
    while (active_item != NULL)
    {
        active_item = before_item;
        if (strcmp(active_item->key, key)){
            if (active_item->next == NULL){
                // If there is no item in row, make new first item
                hTab->variables[hash_key_index] = before_item;
            } else {
                // If there is another item in row, connect it with the item before
                before_item->next = active_item->next;
            }
            // Free item attributes
            free(active_item->key.text);
            free(active_item);
            break;
        }
        active_item = active_item->next;
    }
}

unsigned long THashFunction(char string[]) {
    unsigned long h=0;
    const unsigned char *p;
    for ( p = (const unsigned char *) string; *p!='\0'; p++) {
        h = 65599 * h + *p;
    }
    return h;
}