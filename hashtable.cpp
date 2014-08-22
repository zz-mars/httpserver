#include "hashtable.h"
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpq_hash.h"
#include "utility.h"

// serial number as value
static uint64_t g_u64ElementVal;
// max element number
static uint32_t g_u32MaxElementNum;
// hash space
static struct SHashItem * g_szstHashItems;
// slots number in each hash
static uint32_t g_u32ElementPerHash;
// hash times
static uint32_t g_u32HashTimes;
// primes used for each hash
static uint32_t * g_szu32HashPrimes;
// hash table slots used
static uint32_t g_u32UsedSize;

// memory 
static void * g_pHashTableSpace;

// initialized?
static bool g_bHashInitialized = false;

#define RET_IF_NOT_INIT(ret)   do {    \
    if(!g_bHashInitialized) {   \
        fprintf(stderr, "hash table not initialized yet!\n");   \
        return ret;   \
    }   \
} while(0)

static void GeneratePrimes(void) {
    int iMax = g_u32ElementPerHash;
    iMax -= (iMax%2==0)?1:0;
    uint32_t uiPrimeIdx = 0;
    while(iMax > 0 && uiPrimeIdx < g_u32HashTimes) {
        if(IsPrime(iMax)) {
            g_szu32HashPrimes[uiPrimeIdx++] = iMax;
        }
        iMax-=2;
    }
    assert(uiPrimeIdx == g_u32HashTimes);
}

/* InitHashItems : initialize the hash items in each hash */ 
static void InitHashItems(void) {
    for(uint32_t uiHashIdx=0;uiHashIdx<g_u32HashTimes;uiHashIdx++) {
        struct SHashItem *pHashItems = g_szstHashItems + (uiHashIdx * g_u32ElementPerHash);
        for(uint32_t uiHashItemIdx=0;uiHashItemIdx<g_szu32HashPrimes[uiHashIdx];uiHashItemIdx++) {
            pHashItems[uiHashItemIdx].m_u64Key = 0;
            pHashItems[uiHashItemIdx].m_u64Value = 0;
            pHashItems[uiHashItemIdx].m_iUsed = HASH_ITEM_FREE;
        }
    }
}

/* HashTableInit : initialize hash table
 * Basically this function allocates memory for the hash items,
 * then initialize some variables.
 * Return 0 on success, -1 on error */
int HashTableInit(int iShmKey, uint32_t uiMaxElementNum, uint32_t uiHashTimes = DEFAULT_HASH_TIMES) {
    if(g_bHashInitialized) {
        fprintf(stderr, "already initialized\n");
        return 0;
    }
    if(uiMaxElementNum <= 25000) {
        fprintf(stderr, "uiMaxElementNum too small -> %u\n", uiMaxElementNum);
        return -1;
    }
    // value for k-v start with 1
    // 0 is reserved for non-existing key
    g_u64ElementVal = 1;
    g_u32HashTimes = uiHashTimes;
    g_u32MaxElementNum = (uint32_t)AlignTo((uint64_t)uiMaxElementNum, (uint64_t)g_u32HashTimes);
    g_u32ElementPerHash = g_u32MaxElementNum / g_u32HashTimes;
    g_u32UsedSize = 0;
    int iFlags = 0666;
    // make sure enough memory is allocated
    uint32_t u32MemoryBytes = (g_u32HashTimes + 1) * sizeof(uint32_t) + 
        (g_u32MaxElementNum + 1) * sizeof(struct SHashItem);
    // try to get the existed shm
    int iShmId = shmget(iShmKey, u32MemoryBytes, iFlags & (~IPC_CREAT));
    if(iShmId == -1) {
        // new shm
        iShmId = shmget(iShmKey, u32MemoryBytes, iFlags | IPC_CREAT);
        if(iShmId == -1) {
            fprintf(stderr, "shmget fail -> %s\n", strerror(errno));
            return -1;
        }
    }
    // attach to shm
    g_pHashTableSpace = shmat(iShmId, NULL, 0);
    g_szu32HashPrimes = (uint32_t*)AlignTo((uint64_t)g_pHashTableSpace, sizeof(uint32_t));
    g_szstHashItems = (struct SHashItem*)AlignTo((uint64_t)(g_szu32HashPrimes+g_u32HashTimes), sizeof(SHashItem));
    GeneratePrimes();
    InitHashItems();
    // set initialized
    g_bHashInitialized = true;
    return 0;
}

/* HashTableDestory : destory hash table
 * Basically this function free the space and
 * do some cleanups.. 
 * return 0 on success, -1 if not initialized */
int HashTableDestory(void) {
    RET_IF_NOT_INIT(-1);
    shmdt(g_pHashTableSpace);
    g_bHashInitialized = false;
    g_pHashTableSpace = NULL;
    g_szu32HashPrimes = NULL;
    g_szstHashItems = NULL;
    return 0;
}

/* RawHashTableGet : search for a given key 
 * return pointer to the SHashItem if found,
 * NULL if not found */
static struct SHashItem * RawHashTableGet(char *pKey) {
    RET_IF_NOT_INIT(NULL);
    uint64_t u64HashKey = GetMPQHashFromString(pKey, true);
    for(uint32_t uiHashIdx=0;uiHashIdx<g_u32HashTimes;uiHashIdx++) {
        uint32_t u32HashPos = u64HashKey % g_szu32HashPrimes[uiHashIdx];
        u32HashPos += (uiHashIdx * g_u32ElementPerHash);
        struct SHashItem * pstHashItem = &g_szstHashItems[u32HashPos];
        if(pstHashItem->m_iUsed == HASH_ITEM_TAKEN && 
                pstHashItem->m_u64Key == u64HashKey) {
            return pstHashItem;
        }
    }
    // not found
    return NULL;
}

/* HashTableGet : get from hash table for pKey.
 * Return corresponding u64Value of struct SHashItem if pKey is found,
 * otherwise return 0. */
uint64_t HashTableGet(char *pKey) {
    struct SHashItem * pstHashItem = RawHashTableGet(pKey);
    return pstHashItem==NULL?0:pstHashItem->m_u64Value;
}

/* HashTableDelete : delete a given key
 * return 0 on success, -1 on error */
int HashTableDelete(char *pKey) {
    struct SHashItem * pstHashItem = RawHashTableGet(pKey);
    if(pstHashItem == NULL) {
        return -1;
    }
    pstHashItem->m_u64Key = pstHashItem->m_u64Value = 0;
    pstHashItem->m_iUsed = HASH_ITEM_FREE;
    g_u32UsedSize--;
    return 0;
}

/* HashTableSet : set key 'pKey' to hash table 
 * If key already exists, set it to the new value.
 * Return 0 on success, -1 on error */
int HashTableSet(char *pKey) {
    RET_IF_NOT_INIT(-1);
    uint64_t u64HashKey = GetMPQHashFromString(pKey, true);
    for(uint32_t uiHashIdx=0;uiHashIdx<g_u32HashTimes;uiHashIdx++) {
        uint32_t u32HashPos = u64HashKey % g_szu32HashPrimes[uiHashIdx];
        u32HashPos += (uiHashIdx * g_u32ElementPerHash);
        struct SHashItem * pstHashItem = &g_szstHashItems[u32HashPos];
        if(pstHashItem->m_iUsed == HASH_ITEM_TAKEN) {
            if(pstHashItem->m_u64Key == u64HashKey) {
                g_u32UsedSize--;
                goto set_to_hash;
            }
            continue;
        }
set_to_hash:
        // set to hash if key already exists or free slot is found
        pstHashItem->m_u64Key = u64HashKey;
        pstHashItem->m_u64Value = ++g_u64ElementVal;
        pstHashItem->m_iUsed = HASH_ITEM_TAKEN;
        g_u32UsedSize++;
        return 0;
    }
    // set to hash fail, no free slots
    return -1;
}

/* return used element size of hash table */
uint32_t HashTableGetUsedElementSize(void) {
    RET_IF_NOT_INIT(0);
    return g_u32UsedSize;
}

static int HashTableInfo(void) {
    RET_IF_NOT_INIT(-1);
    printf("---------------- hash table ---------------- \n");
    printf("g_u64ElementVal -> %llu\n", g_u64ElementVal); 
    printf("g_u32MaxElementNum -> %u\n", g_u32MaxElementNum);
    printf("g_u32ElementPerHash -> %u\n", g_u32ElementPerHash);
    printf("g_u32HashTimes -> %u\n", g_u32HashTimes);
    printf("g_u32UsedSize -> %u\n", g_u32UsedSize);
    printf("g_pHashTableSpace -> %0x\n", g_pHashTableSpace);
    printf("g_szu32HashPrimes -> %0x\n", g_szu32HashPrimes);
    printf("g_szstHashItems -> %0x\n", g_szstHashItems);

#ifdef _TEST_PRIMES
    for(int i=0;i<g_u32HashTimes;i++) {
        printf("hash_prime[%u] -> %u\n", i, g_szu32HashPrimes[i]);
    }
#endif
    return 0;
}

enum {
    HASH_OP_GET = 0,
    HASH_OP_SET,
    HASH_OP_DEL,
};

/* HashTableLoadFile : load data from file and insert into hash table */
int HashTableLoadFile(const char *pFile, uint8_t u8Op) {
    RET_IF_NOT_INIT(-1);
    FILE *pFp = fopen(pFile, "r");
    if(!pFp) {
        fprintf(stderr, "open file for reading fail -> %s\n", strerror(errno));
        return -1;
    }
#define TMP_BUF_SZ  1024
    static char szTmpBuf[TMP_BUF_SZ];
    while(fgets(szTmpBuf, sizeof(szTmpBuf), pFp)) {
        szTmpBuf[strlen(szTmpBuf)-1] = '\0';
        switch(u8Op) {
            case HASH_OP_SET:
                if(HashTableSet(szTmpBuf) != 0) {
                    fprintf(stderr, "hash table set fail -> %s\n", szTmpBuf);
                }
                break;
            case HASH_OP_GET:
                if(HashTableGet(szTmpBuf) == 0) {
                    fprintf(stderr, "hash table get fail -> %s\n", szTmpBuf);
                }
                break;
            case HASH_OP_DEL:
                if(HashTableDelete(szTmpBuf) == -1) {
                    fprintf(stderr, "hash table delete fail -> %s\n", szTmpBuf);
                }
                break;
            default:
                fclose(pFp);
                return -1;
        }
    }
    fclose(pFp);
    return 0;
}

#ifdef _TEST_HT

int main(int argc, char *argv[]) {
    if(argc != 4) {
        fprintf(stderr, "Usage : %s [ShmKey] [ElementNum] [init_file]\n", argv[0]);
        return 1;
    }
    int iShmKey = atoi(argv[1]);
    uint32_t uMaxElementNum = (uint32_t)atoi(argv[2]);
    if(HashTableInit(iShmKey, uMaxElementNum) != 0) {
        fprintf(stderr, "Hash table init fail\n");
        return 1;
    }

    HashTableInfo();

#ifdef _TEST_PRIMES
    for(int j=0;j<500;j++) {
        if(IsPrime(j)) {
            printf("%u ", j);
        }
    }
    printf("\n");
#endif
#ifdef _TEST_REINIT
    if(HashTableInit(iShmKey, uMaxElementNum) != 0) {
        fprintf(stderr, "Hash table init fail\n");
        return 1;
    }
#endif

    HashTableLoadFile(argv[3], HASH_OP_SET);
    HashTableInfo();

    HashTableLoadFile(argv[3], HASH_OP_GET);
    HashTableInfo();

    HashTableLoadFile(argv[3], HASH_OP_DEL);
    HashTableInfo();

    HashTableDestory();
    return 0;
}

#endif

