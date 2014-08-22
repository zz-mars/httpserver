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

#define BUDDY_TAKEN_FREE_MASK   0x80000000
#define BUDDY_ORDER_MASK        0x7c000000
#define BUDDY_NUMBER_MASK       0x03ffffff

#define NULL_UNIT_NO            BUDDY_NUMBER_MASK

// inside the flag : 
//     1      +   5   +     26
// free&taken + order + unit_number 

#define BUDDY_ORDER(pBuddyUnit)  ({  \
        uint32_t * _pBuddyUnit = (uint32_t*)(pBuddyUnit); \
        uint32_t _order = (*_pBuddyUnit & BUDDY_ORDER_MASK);    \
        (_order >> 26); })

#define SET_BUDDY_ORDER(pBuddyUnit, buddy_order) do {    \
    uint32_t * _pBuddyUnit = (uint32_t*)(pBuddyUnit); \
    uint32_t _order = (uint32_t)(buddy_order);  \
    _order <<= 26;  \
    *_pBuddyUnit &= (~BUDDY_ORDER_MASK); \
    *_pBuddyUnit |= _order;  \
} while (0)

// is taken?
#define IS_BUDDY_BLOCK_TAKEN(pBuddyUnit)    ({  \
        uint32_t * _pBuddyUnit = (uint32_t*)(pBuddyUnit); \
        ((*_pBuddyUnit & BUDDY_TAKEN_FREE_MASK) == BUDDY_TAKEN_FREE_MASK);})

// is free?
#define IS_BUDDY_BLOCK_FREE(pBuddyUnit) ({  \
        uint32_t * _pBuddyUnit = (uint32_t*)(pBuddyUnit); \
        ((*_pBuddyUnit & BUDDY_TAKEN_FREE_MASK) == 0);})

// set taken
#define SET_BUDDY_BLOCK_TAKEN(pBuddyUnit)    do {    \
    uint32_t * _pBuddyUnit = (uint32_t*)(pBuddyUnit); \
    *_pBuddyUnit |= BUDDY_TAKEN_FREE_MASK; \
} while (0)

// set free
#define SET_BUDDY_BLOCK_FREE(pBuddyUnit)    do {    \
    uint32_t * _pBuddyUnit = (uint32_t*)(pBuddyUnit); \
    *_pBuddyUnit &= (~BUDDY_TAKEN_FREE_MASK); \
} while (0)

#define BUDDY_NEXT_UNIT(pBuddyUnit) ({    \
        uint32_t * _pBuddyUnit = (uint32_t*)(pBuddyUnit); \
        (*_pBuddyUnit & BUDDY_NUMBER_MASK); })

#define SET_BUDDY_NEXT_UNIT(pBuddyUnit, unit_no)  do {    \
    uint32_t * _pBuddyUnit = (uint32_t*)(pBuddyUnit); \
    uint32_t _unitno = (uint32_t)(unit_no); \
    *_pBuddyUnit &= (~BUDDY_NUMBER_MASK);   \
    *_pBuddyUnit |= _unitno;    \
} while (0)

struct SStrBuddy {
    // total mem 
    char * m_pMem;
    uint32_t m_u32MemSize;

    // order of buddy system
    uint32_t m_u32BuddyOrder;
    uint32_t * m_pIOrderArray;

    // mem-unit descriptor 
    uint32_t * m_pMemUnitDescBase;
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

