#include "buddy.h"
#include "utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static inline struct SMemUnitDescriptor * DESC_OF_UNIT_NO(struct SStrBuddy * pSStrBuddy, int iUnitNo) {
    return (pSStrBuddy->m_pMemUnitDescBase + iUnitNo);
}

static inline int UNIT_NO_OF_DESC(struct SStrBuddy * pSStrBuddy, struct SMemUnitDescriptor * pUnit) {
    return (int)(pUnit - pSStrBuddy->m_pMemUnitDescBase);
}

static inline int NEXT_UNIT_NO(struct SStrBuddy * pSStrBuddy, int iUnitNo) {
    struct SMemUnitDescriptor * _sm = DESC_OF_UNIT_NO(pSStrBuddy, iUnitNo);
    return _sm->m_iNext;
}

// memory layout
// SStrBuddy + order_array + desc_array + str_mem_region

static inline void DisplayOrderArray(struct SStrBuddy *pSStrBuddy) {
    printf("---------------- order-array ----------------\n");
    for(uint32_t uiOrder=0;uiOrder<=pSStrBuddy->m_u32BuddyOrder;uiOrder++) {
        uint32_t uiOrderNum = 0;
        printf("order[%2u] -> ", uiOrder);
        int iUnitNo = pSStrBuddy->m_pIOrderArray[uiOrder];
        while(iUnitNo != NULL_UNIT_NO) {
            iUnitNo = NEXT_UNIT_NO(pSStrBuddy, iUnitNo);
            uiOrderNum++;
        }
        printf(" -> %2u\n", uiOrderNum);
    }
    printf("strMemSize -> %u : used -> %u\n", 
            pSStrBuddy->m_u32StrMemSize,
            pSStrBuddy->m_u32StrMemUsed);
}
// init a buddy system with at least iUnitNumber mem-units
// return pointer to struct SStrBuddy on success, 
// NULL on error
struct SStrBuddy * StrBuddyInit(uint32_t uiUnitNumber) {
    if(uiUnitNumber <= 0) {
        return NULL;
    }
    uint32_t u32Order;
    uiUnitNumber = NearestPowerof2(uiUnitNumber, &u32Order);

    printf("order -> %u unit-number -> %u\n", u32Order, uiUnitNumber);

    if(u32Order < 10 || u32Order > BUDDY_MAX_ORDER) {
        fprintf(stderr, "StrBuddyInit -> EINVAL : unit_num -> %u order -> %u\n", 
                uiUnitNumber, u32Order);
        return NULL;
    }

    // make sure enough memory is allocated
    uint32_t u32TotalMemSize = sizeof(struct SStrBuddy) * 2 + 
        sizeof(struct SMemUnitDescriptor *) * (u32Order + 1) +
        sizeof(struct SMemUnitDescriptor) * (uiUnitNumber + 1) +
        BUDDY_UNIT_SZ * (uiUnitNumber + 1);

    char *pTotalMem = (char*)malloc(u32TotalMemSize);
    if(!pTotalMem) {
        return NULL;
    }

    printf("total memory size -> %u\n", u32TotalMemSize);

    struct SStrBuddy * pSStrBuddy = (struct SStrBuddy *)AlignTo((uint64_t)pTotalMem,
            sizeof(struct SStrBuddy));

    printf("sizeof(SStrBuddy) -> %u sizeof(SMemUnitDescriptor*) -> %u sizeof(SMemUnitDescriptor) -> %u\n",
            sizeof(struct SStrBuddy), sizeof(struct SMemUnitDescriptor*), sizeof(struct SMemUnitDescriptor));
    // total mem 
    pSStrBuddy->m_pMem = pTotalMem;
    pSStrBuddy->m_u32MemSize = u32TotalMemSize;
    printf("total_mem -> %0x size -> %u\n", pSStrBuddy->m_pMem, pSStrBuddy->m_u32MemSize);
    // order
    pSStrBuddy->m_u32BuddyOrder = u32Order;
    pSStrBuddy->m_pIOrderArray = (int*)AlignTo((uint64_t)(pSStrBuddy+1), sizeof(int));
    printf("order_array -> %0x order -> %u\n", pSStrBuddy->m_pIOrderArray, pSStrBuddy->m_u32BuddyOrder);
    // mem-unit descriptor array
    pSStrBuddy->m_u32MemUnitDescNum = uiUnitNumber;
    pSStrBuddy->m_pMemUnitDescBase = (struct SMemUnitDescriptor*)AlignTo((uint64_t)(pSStrBuddy->m_pIOrderArray+pSStrBuddy->m_u32BuddyOrder+1), 
            sizeof(struct SMemUnitDescriptor));
    printf("mem_unit_desc -> %0x number -> %u\n", pSStrBuddy->m_pMemUnitDescBase, pSStrBuddy->m_u32MemUnitDescNum);

    // string memeory region
    pSStrBuddy->m_pStrMemBase = (char*)(pSStrBuddy->m_pMemUnitDescBase + pSStrBuddy->m_u32MemUnitDescNum);
    printf("strMemBase -> %0x\n", pSStrBuddy->m_pStrMemBase);
    pSStrBuddy->m_u32StrMemSize = BUDDY_UNIT_SZ * pSStrBuddy->m_u32MemUnitDescNum;
    pSStrBuddy->m_u32StrMemUsed = 0;

    uint32_t u32MetaInfoSize = pSStrBuddy->m_pStrMemBase - pTotalMem;
    uint32_t u32UsedMemSize = u32MetaInfoSize + pSStrBuddy->m_u32StrMemSize;
    printf("total used memory size -> %u\n", u32UsedMemSize);
    printf("meta info memory size -> %u\n", u32MetaInfoSize);
    // initialize
    for(uint32_t uiUnitIdx=0;uiUnitIdx<=pSStrBuddy->m_u32BuddyOrder;uiUnitIdx++) {
        pSStrBuddy->m_pIOrderArray[uiUnitIdx] = NULL_UNIT_NO;
    }
    // set biggest order
    SET_BUDDY_ORDER(&pSStrBuddy->m_pMemUnitDescBase[0], pSStrBuddy->m_u32BuddyOrder);
    // set free
    SET_BUDDY_BLOCK_FREE(&pSStrBuddy->m_pMemUnitDescBase[0]);
    // set next to NULL
    pSStrBuddy->m_pMemUnitDescBase[0].m_iNext = NULL_UNIT_NO;
    // insert to biggest order list
    pSStrBuddy->m_pIOrderArray[pSStrBuddy->m_u32BuddyOrder] = 0;
    DisplayOrderArray(pSStrBuddy);
    return pSStrBuddy;
}

// destroy a buddy system
// return 0 on success, -1 on error
int StrBuddyDestroy(struct SStrBuddy * pSStrBuddy) {
    if(pSStrBuddy->m_pMem) {
        free(pSStrBuddy->m_pMem);
    }
    return 0;
}

// allocate memory for at least iSize bytes from buddy
// return mem-region pointer on success or NULL on error
char * StrBuddyAlloc(struct SStrBuddy * pSStrBuddy, uint32_t uiSize) {
    // align to BUDDY_UNIT_SZ
    uint32_t uiRealSize = (uint32_t)AlignTo((uint64_t)uiSize, BUDDY_UNIT_SZ);
    uint32_t uiUnitsNeeded = uiRealSize / BUDDY_UNIT_SZ;
    uint32_t uiUnitsPower = 0;
    uint32_t uiRealUnitsNeeded = NearestPowerof2(uiUnitsNeeded, &uiUnitsPower);
    // request for too much memory
    if(uiUnitsNeeded > uiRealUnitsNeeded) { 
        return NULL;
    }

    //    printf("req_size -> %u : aligned_size -> %u : units_needed -> %u : adjusted_units -> %u : power -> %u\n",
    //            uiSize, uiRealSize, uiUnitsNeeded, uiRealUnitsNeeded, uiUnitsPower);
    int iToAlloc = NULL_UNIT_NO;
    uint32_t uiPower;
    for(uiPower=uiUnitsPower;uiPower<=pSStrBuddy->m_u32BuddyOrder;uiPower++) {
        iToAlloc = pSStrBuddy->m_pIOrderArray[uiPower];
        if(iToAlloc != NULL_UNIT_NO) {
            break;
        }
    }

    // no mem
    if(iToAlloc == NULL_UNIT_NO) {
        return NULL;
    }
    // update mem usage info
    pSStrBuddy->m_u32StrMemUsed += (uiRealUnitsNeeded * BUDDY_UNIT_SZ);
    // delete from list
    struct SMemUnitDescriptor * pToAlloc = DESC_OF_UNIT_NO(pSStrBuddy, iToAlloc);
    pSStrBuddy->m_pIOrderArray[uiPower] = pToAlloc->m_iNext;

    // split if needed
    for(uint32_t uiPowerIdx=uiUnitsPower;uiPowerIdx<uiPower;uiPowerIdx++) {
        // find buddy
        struct SMemUnitDescriptor * pBuddy = pToAlloc + (1<<uiPowerIdx);
        int iBuddyNo = iToAlloc + (1<<uiPowerIdx);
        // set buddy order
        SET_BUDDY_ORDER(pBuddy, uiPowerIdx);
        // set buddy free
        SET_BUDDY_BLOCK_FREE(pBuddy);
        // insert to free list
        pBuddy->m_iNext = pSStrBuddy->m_pIOrderArray[uiPowerIdx];
        pSStrBuddy->m_pIOrderArray[uiPowerIdx] = iBuddyNo;
    }

    // set the allocated buddy block to new order and taken
    SET_BUDDY_ORDER(pToAlloc, uiUnitsPower);
    SET_BUDDY_BLOCK_TAKEN(pToAlloc);

    return (pSStrBuddy->m_pStrMemBase + BUDDY_UNIT_SZ * iToAlloc);
}

// delete from old list of order 'uiOrder'
// return 0 on success, -1 on error (no entry found)
static inline int DeleteFromOrderList(struct SStrBuddy * pSStrBuddy, 
        int iBuddyNo, uint32_t u32Order) {
    int *iNo = &pSStrBuddy->m_pIOrderArray[u32Order];
    while(*iNo != NULL_UNIT_NO) {
        struct SMemUnitDescriptor * pToDel = DESC_OF_UNIT_NO(pSStrBuddy, *iNo);
        if(*iNo == iBuddyNo) {
            *iNo = pToDel->m_iNext;
            return 0;
        }
        iNo = &pToDel->m_iNext;
    }
    return -1;
}

// free a mem-region into the buddy
// return 0 on success, -1 on error
int StrBuddyFree(struct SStrBuddy * pSStrBuddy, char * pMem) {
    if((pMem - pSStrBuddy->m_pStrMemBase) % BUDDY_UNIT_SZ != 0) {
        fprintf(stderr, "strBuddyFree : invalid addr -> %0x\n", pMem);
        return -1;
    }
    int iUnitNo = (pMem - pSStrBuddy->m_pStrMemBase) / BUDDY_UNIT_SZ;
    struct SMemUnitDescriptor * pUnit = pSStrBuddy->m_pMemUnitDescBase + iUnitNo;
    if(IS_BUDDY_BLOCK_FREE(pUnit)) {
        fprintf(stderr, "strBuddyFree : try to free a free mem -> %0x\n", pMem);
        return -1;
    }
    uint32_t uiOrder=BUDDY_ORDER(pUnit);
    // update mem usage info
    pSStrBuddy->m_u32StrMemUsed -= (1<<uiOrder) * BUDDY_UNIT_SZ;
    // try to combine
    for(;uiOrder<pSStrBuddy->m_u32BuddyOrder;uiOrder++) {
        // find buddy
        int iBuddyNo = iUnitNo ^ (1 << uiOrder);
        struct SMemUnitDescriptor * pBuddy = DESC_OF_UNIT_NO(pSStrBuddy, iBuddyNo); 
        // free & same order
        if(IS_BUDDY_BLOCK_FREE(pBuddy) &&
                BUDDY_ORDER(pBuddy) == uiOrder) {
            // delete buddy from old free list
            if(DeleteFromOrderList(pSStrBuddy, iBuddyNo, uiOrder) != 0) {
                return -1;
            }
            // combine
            if(iBuddyNo < iUnitNo) {
                pUnit = pBuddy;
                iUnitNo = iBuddyNo;
            }
            SET_BUDDY_BLOCK_FREE(pUnit);
            SET_BUDDY_ORDER(pUnit, uiOrder+1);
        } else {
            break;
        }
    }
    // insert to current order list
    // need set to free state 
    // because the for-loop above may not be executed
    SET_BUDDY_BLOCK_FREE(pUnit);
    pUnit->m_iNext = pSStrBuddy->m_pIOrderArray[uiOrder];
    pSStrBuddy->m_pIOrderArray[uiOrder] = iUnitNo;
    return 0;
}

#ifdef _TEST_BUDDY
int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage : %s [input_file]\n", argv[0]);
        return 1;
    }

    struct SStrBuddy * pSStrBuddy = StrBuddyInit(DEFAULT_BUDDY_UNIT_NUM);
    if(!pSStrBuddy) {
        fprintf(stderr, "buddy init fail!\n");
        exit(EXIT_FAILURE);
    }

#ifdef _TEST_FILE
    FILE *pFp = fopen(argv[1], "r");
    if(!pFp) {
        perror("open file for read ");
        return 1;
    }

    static char szLineBuf[1024] = {0};
#define LINE_NR_MAX (1<<25)
    static char * pszLinePointers[LINE_NR_MAX];
    int iLineNr = 0;
    while(fgets(szLineBuf, 1024, pFp)) {
        uint32_t uiLen = (uint32_t)strlen(szLineBuf);
       // printf("alloc -> %u\n", uiLen);
        pszLinePointers[iLineNr] = StrBuddyAlloc(pSStrBuddy, uiLen);
        if(pszLinePointers[iLineNr] == NULL) {
            fprintf(stderr, "allocate for %u line fail\n", iLineNr);
            break;
        }
        strncpy(pszLinePointers[iLineNr++], szLineBuf, uiLen);
        memset(szLineBuf, 0, 1024);
    }
    DisplayOrderArray(pSStrBuddy);
#endif


    uint32_t uiCount = 0;
    while(StrBuddyAlloc(pSStrBuddy, 1380) != NULL) {
        uiCount++;
    }
    printf("alloc 1380 -> cannot fufiled -> %u\n", uiCount);
    DisplayOrderArray(pSStrBuddy);

    /*
    uiCount = 0;
    while(StrBuddyAlloc(pSStrBuddy, 768) != NULL) {
        uiCount++;
    }
    printf("alloc 768 -> cannot fufiled -> %u\n", uiCount);
    DisplayOrderArray(pSStrBuddy);

    uiCount = 0;
    while(StrBuddyAlloc(pSStrBuddy, 250) != NULL) {
        uiCount++;
    }
    printf("alloc 250 -> cannot fufiled -> %u\n", uiCount);
    DisplayOrderArray(pSStrBuddy);

    uiCount = 0;
    while(StrBuddyAlloc(pSStrBuddy, 120) != NULL) {
        uiCount++;
    }
    printf("alloc 120 -> cannot fufiled -> %u\n", uiCount);
    DisplayOrderArray(pSStrBuddy);

    uiCount = 0;
    while(StrBuddyAlloc(pSStrBuddy, 60) != NULL) {
        uiCount++;
    }
    printf("alloc 60 -> cannot fufiled -> %u\n", uiCount);
    DisplayOrderArray(pSStrBuddy);

    uiCount = 0;
    while(StrBuddyAlloc(pSStrBuddy, 30) != NULL) {
        uiCount++;
    }
    printf("alloc 30 -> cannot fufiled -> %u\n", uiCount);
    DisplayOrderArray(pSStrBuddy);
    */

#ifdef _TEST_FILE
    // free
    for(int iLineIdx=0;iLineIdx<iLineNr;iLineIdx++) {
        if(pszLinePointers[iLineIdx] == NULL) {
            break;
        }
    //    fprintf(stderr, "%s",  pszLinePointers[iLineIdx]);
        if(StrBuddyFree(pSStrBuddy, pszLinePointers[iLineIdx]) != 0) {
            fprintf(stderr, "free for %u line fail\n", iLineIdx);
            break;
        }
    }

    DisplayOrderArray(pSStrBuddy);
    fclose(pFp);
#endif

    if(StrBuddyDestroy(pSStrBuddy) != 0) {
        fprintf(stderr, "buddy destroy fail!\n");
    }


    exit(EXIT_SUCCESS);
}
#endif

