/**
 * Implementation of imperative language IFJ2019 translator
 * @file symtable.h
 * @author ...
 * @brief Symtable interface
 */

#ifndef VUT_FIT_IFJ2019_SYMTABLE_H
#define VUT_FIT_IFJ2019_SYMTABLE_H

#include <mach/mach_types.h>
#include "dynamic-string.h"
#include "scanner.h"
#include <string.h>

/**
 * @enum IFJ2019 Data types
 */
typedef enum {
    TypeInteger,
    TypeDouble,
    TypeString,
    TypeBool,
    TypeUndefined
} varType_t;

/**
 * @struct Representation of a symtable item
 */
typedef struct hTabItem {
    dynamicString_t key;
    varType_t type;
    bool defined;
    tokenAttribute_t value;
    struct variable* next;
} hTabItem_t;

/**
 * @struct Representation of a hash table
 */
typedef struct {
    unsigned long size;
    hTabItem_t* variables;
} hashTable;

/**
 * @brief Initialises to and empty hash table with capacity = size
 *
 * @param hTab Hash table
 * @param size Hash table size
 * @return Whether the initialization was successful
 */
bool TInit(hashTable* hTab, unsigned long size);

/**
 * @brief Frees all items and the hash table
 *
 * @param hTab Hash table to be freed
 */
void TFree(hashTable* hTab);
/**
 * @brief Inserts an item to hash table
 *
 * @param hTab Hash table
 * @param item Item to be inserted
 * @return Added item, NULL if allocation failed
 */
hTabItem_t* TInsert(hashTable* hTab, hTabItem_t item);

/**
 * @brief Find an item by its key
 *
 * @param hTab Hash table
 * @param key Items key
 * @return Existing item
 */
hTabItem_t* TSearch(hashTable* hTab, dynamicString_t key);

/**
 * @brief Deletes an item from hash table
 *
 * @param hTab Hash table
 * @param key Items key
 */
void TDelete(hashTable* hTab, dynamicString_t key);

/**
 * @brief Hash function
 *
 * @param string String for hashing
 * @return Hash value
 */
unsigned long THashFunction(char string[]);

#endif //VUT_FIT_IFJ2019_SYMTABLE_H