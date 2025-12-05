#ifndef _H_TEST386
#define _H_TEST386

#include "x86/x86.h"

class PlainCPU : public x86Internal {
  public:
    PlainCPU(int mem_size) : x86Internal(mem_size) {}
    ~PlainCPU() override {}
    int get_hard_irq() override { return 0; }
    int get_hard_intno() override { return 0; }
    int ioport_read(int port_num) override;
    void ioport_write(int port_num, int data) override;
};

class Test386 {
    PlainCPU *cpu = nullptr;
public:
    Test386(int mem_size) {
        cpu = new PlainCPU(mem_size);
    }
    ~Test386() {
        delete cpu;
    }
    int load(std::string path, int offset) {
        FILE *f = fopen(path.c_str(), "rb");
        fseek(f, 0, SEEK_END);
        const int size = ftell(f);
        fseek(f, 0, SEEK_SET);
        auto buffer = new uint8_t[size];
        auto __     = fread(buffer, size, 1, f);
    
        printf("load %d bytes at 0x%x\n", size, offset);
        for (int i = 0; i < size; i++) {
            cpu->st8_phys(offset + i, buffer[i]);
        }
        fclose(f);
    
        return size;
    }
    void setup() {
        load("../test386.asm/test386.bin", 0x000f0000);

        printf("\n\n*******************************\n");
        printf("*******************************\n");
        printf("*******************************\n");
        printf("****** Run test386 suite ******\n");
        printf("*******************************\n\n\n");
    }
    void cycle() {
        uint64_t cycles = cpu->cycles + 100000;
        while (cpu->cycles < cycles) {
            try {
                cpu->fetch_decode_execute(cycles - cpu->cycles);
                if (cpu->halted)
                    break;
            } catch (Interrupt) {}
        }
    }
};

int PlainCPU::ioport_read(int port_num) {
    int port = port_num & (1024 - 1);
    printf("*** ioport_read 0x%04x\n", port);
    return 0xff;
}
void PlainCPU::ioport_write(int port_num, int data) {
    int port = port_num & (1024 - 1);
    if (port == 0x0190) { // default POST_PORT in test386
        printf("*** ioport_write 0x%04x : 0x%08x\n", port, data);
    } else { // any other value considered OUT_PORT
        printf("%c", data);
    }
}

#endif // _H_TEST386
