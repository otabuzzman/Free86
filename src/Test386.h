#ifndef _H_TEST386
#define _H_TEST386

#include "PC.h"

class Test386 : public PC {
public:
    Test386(x86Internal cpu, int mem_size) : PC(x86Internal cpu, int mem_size) {}
    ~Test386() {}
    void setup() {
        load("../test386.asm/test386.bin", 0x000f0000);

        printf("\n\n*******************************\n");
        printf("*******************************\n");
        printf("*******************************\n");
        printf("****** Run test386 suite ******\n");
        printf("*******************************\n\n\n");
    }
    void cycle() {
        int cycles_requested = cpu->cycles_processed + 100000;
        while (cpu->cycles_processed < cycles_requested) {
            try {
                cpu->fetch_decode_execute(cycles_requested - cpu->cycles_processed);
                if (cpu->halted)
                    break;
            } catch (Interrupt) {}
        }
    }
};

class PlainCPU : public x86Internal {
  public:
    PlainCPU(int mem_size) : x86Internal(mem_size) {}
    ~PlainCPU() {}
    int get_hard_irq() { return 0; }
    int get_hard_intno() { return 0; }
    int ioport_read(int mem8_loc);
    void ioport_write(int mem8_loc, int data);
};
int PlainCPU::ioport_read(int mem8_loc) {
    int port = mem8_loc & (1024 - 1);
    printf("*** ioport_read 0x%04x\n", port);
    return 0xff;
}
void PlainCPU::ioport_write(int mem8_loc, int data) {
    int port = mem8_loc & (1024 - 1);
    if (port == 0x0190) { // default POST_PORT in test386
        printf("*** ioport_write 0x%04x : 0x%08x\n", port, data);
    } else { // any other value considered OUT_PORT
        printf("%c", data);
    }
}

#endif // _H_TEST386
