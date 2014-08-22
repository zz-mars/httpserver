/* grantchen - 2014/08/21
 * Buddy allocator for string
 * config BUDDY_UNIT_SZ to fufill your need
 * */
#ifndef _STR_ALLOC_H
#define _STR_ALLOC_H

#include <stdint.h>

#define BUDDY_UNIT_SZ           16

// MAX_BUDDY_UNIT_NUM -> 2^26 -> 64M
#define MAX_BUDDY_UNIT_NUM      (1<<26)
#define DEFAULT_BUDDY_UNIT_NUM  (1<<25)

#define BUDDY_MAX_ORDER         32

#define BUDDY_TAKEN             0x80
#define BUDDY_ORDER_MASK        0x7f

#define NULL_UNIT_NO    -1

// encode the unit_no & order & free&taken flag in one uint32_t
// make most use of the memory
#pragma pack(1)
struct SMemUnitDescriptor {
//    struct SMemUnitDescriptor * next;
    // next unit number in the list, 
    // set to -1 if this is the last one
    int m_iNext;
    // free&taken flag and order are encoded in m_u8Flags
    uint8_t m_u8Flags;
};
#pragma pack()

#define BUDDY_ORDER(pBuddyUnit)  ({  \
        struct SMemUnitDescriptor * _pBuddyUnit = (pBuddyUnit); \
        (uint32_t)(_pBuddyUnit->m_u8Flags & BUDDY_ORDER_MASK);})

#define SET_BUDDY_ORDER(pBuddyUnit, buddy_order) do {    \
    struct SMemUnitDescriptor * _pBuddyUnit = (pBuddyUnit); \
    uint8_t _order = (uint8_t)(buddy_order);  \
    _pBuddyUnit->m_u8Flags &= (~BUDDY_ORDER_MASK); \
    _pBuddyUnit->m_u8Flags |= _order;  \
} while (0)

// is taken?
#define IS_BUDDY_BLOCK_TAKEN(pBuddyUnit)    ({  \
        struct SMemUnitDescriptor * _pBuddyUnit = (pBuddyUnit); \
        ((_pBuddyUnit->m_u8Flags & BUDDY_TAKEN) == BUDDY_TAKEN);})

// is free?
#define IS_BUDDY_BLOCK_FREE(pBuddyUnit) ({  \
        struct SMemUnitDescriptor * _pBuddyUnit = (pBuddyUnit); \
        ((_pBuddyUnit->m_u8Flags & BUDDY_TAKEN) == 0);})

// set taken
#define SET_BUDDY_BLOCK_TAKEN(pBuddyUnit)    do {    \
    struct SMemUnitDescriptor * _pBuddyUnit = (pBuddyUnit); \
    _pBuddyUnit->m_u8Flags |= BUDDY_TAKEN; \
} while (0)

// set free
#define SET_BUDDY_BLOCK_FREE(pBuddyUnit)    do {    \
    struct SMemUnitDescriptor * _pBuddyUnit = (pBuddyUnit); \
    _pBuddyUnit->m_u8Flags &= (~BUDDY_TAKEN); \
} while (0)

struct SStrBuddy {
    // total mem 
    char * m_pMem;
    uint32_t m_u32MemSize;

    // order of buddy system
    uint32_t m_u32BuddyOrder;
    int * m_pIOrderArray;

    // mem-unit descriptor 
    struct SMemUnitDescriptor * m_pMemUnitDescBase;
    uint32_t m_u32MemUnitDescNum;

    // str allocation region
    char * m_pStrMemBase;
    // str allocation size
    uint32_t m_u32StrMemSize;

    // allocated size
    uint32_t m_u32StrMemUsed;
};

// init a buddy system with at least iUnitNumber mem-units
// return pointer to struct SStrBuddy on success, 
// NULL on error
struct SStrBuddy * StrBuddyInit(uint32_t uiUnitNumber);

// destroy a buddy system
// return 0 on success, -1 on error
int StrBuddyDestroy(struct SStrBuddy * pSStrBuddy);

// allocate memory for at least iSize bytes from buddy
// return mem-region pointer on success or NULL on error
char * StrBuddyAlloc(struct SStrBuddy * pSStrBuddy, uint32_t uiSize);

// free a mem-region into the buddy
// return 0 on success, -1 on error
int StrBuddyFree(struct SStrBuddy * pSStrBuddy, char * pMem);

// single instance initialized in main
extern struct SStrBuddy * g_pSStrBuddy;

#endif

