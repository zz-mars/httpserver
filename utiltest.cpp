#include "utility.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage %s [number]\n", argv[0]);
        return 1;
    }
    uint32_t uNum = atoi(argv[1]);

    uint32_t uOrder;
    uint32_t uNextPowerof2 = NextPowerof2(uNum, &uOrder);
    printf("nextpowerof2 -> %u order -> %u\n", uNextPowerof2, uOrder);
    return 0;
}

