#ifndef _HASH_T_H
#define _HASH_T_H

#include <stdint.h>

#define DEFAULT_HASH_TIMES  50

enum {
    HASH_ITEM_FREE = 0,
    HASH_ITEM_TAKEN
};
// mpq hash as key
struct SHashItem {
    uint64_t m_u64Key;
    uint64_t m_u64Value;
    int m_iUsed;
};

/* HashTableInit : initialize hash table
 * Basically this function allocates memory for the hash items,
 * then initialize some variables.
 * Return 0 on success, -1 on error */
int HashTableInit(int iShmKey, uint32_t uiMaxElementNum, uint32_t uiHashTimes);

/* HashTableDestory : destory hash table
 * Basically this function free the space and
 * do some cleanups.. 
 * return 0 on success, -1 if not initialized */
int HashTableDestory(void);

/* HashTableGet : get from hash table for pKey.
 * Return corresponding u64Value of struct SHashItem if pKey is found,
 * otherwise return 0. */
uint64_t HashTableGet(char *pKey);

/* HashTableSet : set key 'pKey' to hash table 
 * If key already exists, set it to the new value.
 * Return 0 on success, -1 on error */
int HashTableSet(char *pKey);

/* HashTableDelete : delete a given key
 * return 0 on success, -1 on error */
int HashTableDelete(char *pKey);

/* return used element size of hash table */
uint32_t HashTableGetUsedElementSize(void);

/* HashTableLoadFile : load data from file and insert into hash table */
int HashTableLoadFile(const char *file);

#endif
