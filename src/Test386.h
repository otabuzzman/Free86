#ifndef _H_TEST386
#define _H_TEST386

#include "PC.h"

class Test386 : public PC {
public:
    void init() {
        printf("load file\n");
        load("../test386.asm/test386.bin", 0x000f0000);
    }

    void start() {
        printf("\n\n*******************************\n");
        printf("*******************************\n");
        printf("*******************************\n");
        printf("****** Run test386 suite ******\n");
        printf("*******************************\n\n\n");
    }
};

#endif // _H_TEST386
