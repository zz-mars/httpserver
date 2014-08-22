#ifndef _Z_UTIL_H
#define _Z_UTIL_H

#include <stdint.h>
#include <assert.h>
#include <math.h>

static inline bool IsPrime(int iNum) {
    if(iNum <= 1) {
        return false;
    } else if(iNum == 2) {
        return true;
    } else if(iNum % 2 == 0) {
        return false;
    }
    int iSqrt = sqrt(iNum);
    for(int iDivisor=3;iDivisor<=iSqrt;iDivisor+=2) {
        if(iNum % iDivisor == 0) {
            return false;
        }
    }
    return true;
}

static inline uint64_t AlignTo(uint64_t uVal, uint64_t uAlign) {
    return ((uVal + uAlign - 1) / uAlign) * uAlign;
}

static inline int IsPowerOf2(uint32_t uVal) {
    return !(uVal & (uVal - 1));
}

#define MASK_1  0x55555555
#define MASK_2  0x33333333
#define MASK_4  0x0f0f0f0f
#define MASK_8  0x00ff00ff
#define MASK_16 0x0000ffff
static inline uint32_t NumberOf1(uint32_t x) {
    x = (x & MASK_1) + ((x & ~MASK_1) >> 1);
    x = (x & MASK_2) + ((x & ~MASK_2) >> 2);
    x = (x & MASK_4) + ((x & ~MASK_4) >> 4);
    x = (x & MASK_8) + ((x & ~MASK_8) >> 8);
    return (x & MASK_16) + ((x & ~MASK_16) >> 16);
}

// assume uVal is power of 2
static inline int Log2Of(uint32_t uVal) {
    if(!IsPowerOf2(uVal)) {
        return -1;
    }
    return NumberOf1(uVal-1);
}

static inline uint32_t NextPowerof2(uint32_t uVal, uint32_t * pOrder) {
    if(IsPowerOf2(uVal)) {
        *pOrder = NumberOf1(uVal-1);
        return uVal;
    }
    uVal |= (uVal>>1);
    uVal |= (uVal>>2);
    uVal |= (uVal>>4);
    uVal |= (uVal>>8);
    uVal |= (uVal>>16);
    *pOrder = NumberOf1(uVal);
    return (uVal + 1);
}


// find the nearest power-of-2 of uVal
// try to find the value above uVal first, 
// if it exceeds (1<<31), cut it as (1<<31)
static inline uint32_t NearestPowerof2(uint32_t uVal, uint32_t * pOrder) {
    int i;
    for(i=31;i>=0;i--) {
        uint32_t uiPowerOf2i = (1<<i);
        if(uVal == uiPowerOf2i) {
            *pOrder = (uint32_t)i;
            return uiPowerOf2i;
        } else if(uVal & uiPowerOf2i) {
            break;
        }
    }
    *pOrder = i==31?31:(uint32_t)(i+1);
    return (1<<(*pOrder));
}

#endif

