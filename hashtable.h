#ifndef _HASH_T_H
#define _HASH_T_H

#include <stdint.h>
#include "buddy.h"
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpq_hash.h"
#include "utility.h"

#define DEFAULT_HASH_TIMES  50

enum {
    HASH_ITEM_FREE = 0,
    HASH_ITEM_TAKEN
};
#pragma pack(1)
struct SHashItem {
    uint64_t m_u64Key;
    char * m_pValue;
    uint8_t m_uiUsed;
};
#pragma pack()

#define RET_IF_NOT_INIT(ret)   do {    \
    if(!m_bInitialized) {   \
        fprintf(stderr, "hash table not initialized yet!\n");   \
        return ret;   \
    }   \
} while(0)

#define RET_IF_INITED(ret)   do {    \
    if(m_bInitialized) {   \
        fprintf(stderr, "hash table already initialized!\n");   \
        return ret;   \
    }   \
} while(0)

#define INT_FATAL_ERROR -2

class C2DHashTable {
    public:

        static C2DHashTable & GetInstance() {
            static C2DHashTable soHashTable;
            return soHashTable;
        }

        /* Init : initialize hash table
         * Basically this function allocates memory for the hash items,
         * then initialize some variables.
         * Return 0 on success, -1 on error */
        int Init(int iShmKey, uint32_t uiMaxElementNum, uint32_t uiHashTimes);

        ~C2DHashTable() {
            if(m_bInitialized) {
                if(m_pTotalMem != NULL) {
                    shmdt(m_pTotalMem);
                }

                if(m_pBuddy) {
                    StrBuddyDestroy(m_pBuddy);
                }
            }
        }

        /* Get : get from hash table for pKey.
         * Return corresponding u64Value of struct SHashItem if pKey is found,
         * otherwise return 0. */
        char * Get(char *pKey);

        /* Set : set key 'pKey' to hash table 
         * If key already exists, set it to the new value.
         * Return 0 on success, -1 on error */
        int Set(char *pKey, char *pVal);

        /* Delete : delete a given key
         * return 0 on success, -1 on error */
        int Delete(char *pKey);

        /* return used element size of hash table */
        uint32_t GetUsedElementSize(void);

        /* LoadFile : load data from file and insert into hash table */
        int LoadFile(const char *file, C2DHashTable & oHashTable, uint8_t op);

        /* Info : display hash table info */
        int Info(void);

    private:

        void GeneratePrimes(void);
        void InitHashItems(void);

        struct SHashItem * RawGet(char * pKey);

        bool m_bInitialized;
        void * m_pTotalMem;
        uint32_t m_u32ElementPerHash;
        uint32_t m_u32ElementNum;
        struct SHashItem * m_pHashItems;
        uint32_t m_u32HashTimes;
        uint32_t * m_pHashPrimes;
        uint32_t m_u32UsedElement;

        struct SStrBuddy * m_pBuddy;
};

#endif
