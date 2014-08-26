#include "hashtable.h"

void C2DHashTable::GeneratePrimes(void) {
    int iMax = m_u32ElementPerHash;
    iMax -= (iMax%2==0)?1:0;
    uint32_t uiPrimeIdx = 0;
    while(iMax > 0 && uiPrimeIdx < m_u32HashTimes) {
        if(IsPrime(iMax)) {
            m_pHashPrimes[uiPrimeIdx++] = iMax;
        }
        iMax-=2;
    }
    assert(uiPrimeIdx == m_u32HashTimes);
}

/* InitHashItems : initialize the hash items in each hash */ 
void C2DHashTable::InitHashItems(void) {
    for(uint32_t uiHashIdx=0;uiHashIdx<m_u32HashTimes;uiHashIdx++) {
        struct SHashItem *pHashItems = m_pHashItems + (uiHashIdx * m_u32ElementPerHash);
        for(uint32_t uiHashItemIdx=0;uiHashItemIdx<m_pHashPrimes[uiHashIdx];uiHashItemIdx++) {
            pHashItems[uiHashItemIdx].m_u64Key = NULL;
            pHashItems[uiHashItemIdx].m_pValue = NULL;
            pHashItems[uiHashItemIdx].m_uiUsed = HASH_ITEM_FREE;
        }
    }
}

/* Init : initialize hash table
 * Basically this function allocates memory for the hash items,
 * then initialize some variables.
 * Return 0 on success, -1 on error */
int C2DHashTable::Init(int iShmKey, uint32_t uiMaxElementNum, uint32_t uiHashTimes = DEFAULT_HASH_TIMES) {
    RET_IF_INITED(0);
    m_bInitialized = false;
    if(uiMaxElementNum <= 25000) {
        fprintf(stderr, "uiMaxElementNum too small -> %u\n", uiMaxElementNum);
        return -1;
    }
    m_u32HashTimes = uiHashTimes;
    m_u32ElementNum = (uint32_t)AlignTo((uint64_t)uiMaxElementNum, (uint64_t)m_u32HashTimes);
    m_u32ElementPerHash = m_u32ElementNum / m_u32HashTimes;
    m_u32UsedElement = 0;
    int iFlags = 0666;
    // make sure enough memory is allocated
    uint32_t u32MemoryBytes = (m_u32HashTimes + 1) * sizeof(uint32_t) + 
        (m_u32ElementNum + 1) * sizeof(struct SHashItem);
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
    m_pTotalMem = shmat(iShmId, NULL, 0);
    m_pHashPrimes = (uint32_t*)AlignTo((uint64_t)m_pTotalMem, sizeof(uint32_t));
    m_pHashItems = (struct SHashItem*)AlignTo((uint64_t)(m_pHashPrimes+m_u32HashTimes), sizeof(SHashItem));
    GeneratePrimes();
    InitHashItems();
    // buddy system
    m_pBuddy = StrBuddyInit(m_u32ElementNum * 5 / 4);
    if(!m_pBuddy) {
        return INT_FATAL_ERROR;
    }
    // set initialized
    m_bInitialized = true;
    return 0;
}

/* RawGet : search for a given key 
 * return pointer to the SHashItem if found,
 * NULL if not found */
struct SHashItem * C2DHashTable::RawGet(char *pKey) {
    RET_IF_NOT_INIT(NULL);
    uint64_t u64HashKey = GetMPQHashFromString(pKey, true);
    for(uint32_t uiHashIdx=0;uiHashIdx<m_u32HashTimes;uiHashIdx++) {
        uint32_t u32HashPos = u64HashKey % m_pHashPrimes[uiHashIdx];
        u32HashPos += (uiHashIdx * m_u32ElementPerHash);
        struct SHashItem * pstHashItem = &m_pHashItems[u32HashPos];
        if(pstHashItem->m_uiUsed == HASH_ITEM_TAKEN && 
                pstHashItem->m_u64Key == u64HashKey) {
            return pstHashItem;
        }
    }
    // not found
    return NULL;
}

/* Get : get value from hash table for pKey.
 * Return corresponding m_pValue of struct SHashItem if pKey is found,
 * otherwise return NULL. */
char * C2DHashTable::Get(char *pKey) {
    struct SHashItem * pstHashItem = RawGet(pKey);
    return pstHashItem==NULL?NULL:pstHashItem->m_pValue;
}

/* Delete : delete a given key
 * return 0 on success, -1 on error */
int C2DHashTable::Delete(char *pKey) {
    struct SHashItem * pstHashItem = RawGet(pKey);
    if(pstHashItem == NULL) {
        return -1;
    }
    // free value mem into buddy system
    if(StrBuddyFree(m_pBuddy, pstHashItem->m_pValue) != 0) {
        // fatal error
        return INT_FATAL_ERROR;
    }
    // reset key & value
    pstHashItem->m_u64Key = 0;
    pstHashItem->m_pValue = NULL;
    pstHashItem->m_uiUsed = HASH_ITEM_FREE;
    // update usage info
    m_u32UsedElement--;
    return 0;
}

/* Set : set key 'pKey' to hash table 
 * If key already exists, set it to the new value.
 * Return 0 on success, -1 on error */
int C2DHashTable::Set(char *pKey, char *pVal) {
    RET_IF_NOT_INIT(-1);
    char * pOldValue = NULL;
    uint64_t u64HashKey = GetMPQHashFromString(pKey, true);
    for(uint32_t uiHashIdx=0;uiHashIdx<m_u32HashTimes;uiHashIdx++) {
        uint32_t u32HashPos = u64HashKey % m_pHashPrimes[uiHashIdx];
        u32HashPos += (uiHashIdx * m_u32ElementPerHash);
        struct SHashItem * pstHashItem = &m_pHashItems[u32HashPos];
        if(pstHashItem->m_uiUsed == HASH_ITEM_TAKEN) {
            if(pstHashItem->m_u64Key == u64HashKey) {
                // old value need free
                pOldValue = pstHashItem->m_pValue;
                goto set_to_hash;
            }
            continue;
        }
set_to_hash:
        // set to hash if key already exists or free slot is found
        pstHashItem->m_u64Key = u64HashKey;
        uint32_t uValueLen = strlen(pVal);
        pstHashItem->m_pValue = StrBuddyAlloc(m_pBuddy, uValueLen + 1);
        if(!pstHashItem->m_pValue) {
            // set to old value and fail
            pstHashItem->m_pValue = pOldValue;
            return -1;
        }
        // copy value
        strncpy(pstHashItem->m_pValue, pVal, uValueLen);
        pstHashItem->m_pValue[uValueLen] = '\0';
        if(pOldValue) {
            if(StrBuddyFree(m_pBuddy, pOldValue) != 0) {
                // fatal error
                return INT_FATAL_ERROR;
            }
        } else {
            pstHashItem->m_uiUsed = HASH_ITEM_TAKEN;
            m_u32UsedElement++;
        }
        return 0;
    }
    // set to hash fail, no free slots
    return -1;
}

/* return used element size of hash table */
uint32_t C2DHashTable::GetUsedElementSize(void) {
    RET_IF_NOT_INIT(0);
    return m_u32UsedElement;
}

int C2DHashTable::Info(void) {
    RET_IF_NOT_INIT(-1);
    printf("---------------- hash table ---------------- \n");
    printf("m_u32ElementNum -> %u\n", m_u32ElementNum);
    printf("m_u32ElementPerHash -> %u\n", m_u32ElementPerHash);
    printf("m_u32HashTimes -> %u\n", m_u32HashTimes);
    printf("m_u32UsedElement -> %u\n", m_u32UsedElement);

    DisplayOrderArray(m_pBuddy);
#ifdef _TEST_PRIMES
    for(uint32_t i=0;i<m_u32HashTimes;i++) {
        printf("hash_prime[%u] -> %u\n", i, m_pHashPrimes[i]);
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
int C2DHashTable::LoadFile(const char *pFile, C2DHashTable & oHashTable, uint8_t u8Op) {
    RET_IF_NOT_INIT(-1);
    FILE *pFp = fopen(pFile, "r");
    if(!pFp) {
        fprintf(stderr, "open file for reading fail -> %s\n", strerror(errno));
        return -1;
    }
#define TMP_BUF_SZ  1024
    static char szTmpBuf[TMP_BUF_SZ];
    uint32_t uOpFail = 0;
    while(fgets(szTmpBuf, sizeof(szTmpBuf), pFp)) {
        szTmpBuf[strlen(szTmpBuf)-1] = '\0';
        char *pKey = szTmpBuf;
        char *pVal = pKey;
        while(*pVal) {
            if(*pVal == ' ' || *pVal == '\t') {
                break;
            }
            pVal++;
        }
        *pVal++ = '\0';
        while(*pVal == ' ' || *pVal == '\t') pVal++;

        int iValLen = strlen(pVal);
        if(iValLen > 1) {
            if(pVal[iValLen-1] == '\n') {
                pVal[iValLen-1] = '\0';
            }
        } else {
            pVal = "default value";
        }

        switch(u8Op) {
            case HASH_OP_SET:
                if(oHashTable.Set(pKey, pVal) != 0) {
                    uOpFail++;
                   // fprintf(stderr, "hash table set fail : k -> %s v -> %s\n", 
                   //        pKey, pVal);
                }
                break;
            case HASH_OP_GET:
                pVal = oHashTable.Get(pKey);
                if(pVal == NULL) {
                    uOpFail++;
                   // fprintf(stderr, "hash table get fail -> %s\n", pKey);
#ifdef _GET_DETAIL_INFO
                } else {
                    printf("get ok : k : %s v %s\n", pKey, pVal);
#endif
                }
                break;
            case HASH_OP_DEL:
                if(oHashTable.Delete(pKey) == -1) {
                    uOpFail++;
                   // fprintf(stderr, "hash table delete fail -> %s\n", szTmpBuf);
                }
                break;
            default:
                fclose(pFp);
                return -1;
        }
    }
    printf("op fail -> %u\n", uOpFail);
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

    C2DHashTable & oHashTable = C2DHashTable::GetInstance();
    if(oHashTable.Init(iShmKey, uMaxElementNum) != 0) {
        return 1;
    }

    oHashTable.LoadFile(argv[3], oHashTable, HASH_OP_SET);
    oHashTable.Info();

    oHashTable.LoadFile(argv[3], oHashTable, HASH_OP_GET);
    oHashTable.Info();

    oHashTable.LoadFile(argv[3], oHashTable, HASH_OP_DEL);
    oHashTable.Info();

    return 0;
}

#endif

