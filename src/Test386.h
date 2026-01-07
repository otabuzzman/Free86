#ifndef TEST386_H
#define TEST386_H

#include <iostream>

#include "x86/free86.h"

class PlainCPU : public Free86 {
  public:
    PlainCPU(int memory_size) : Free86(memory_size) {}
    ~PlainCPU() override {}
    int get_irq() override { return 0; }
    int get_iid() override { return 0; }
    int io_read(int port) override;
    void io_write(int port, int data) override;
};

class Test386 {
  public:
    Test386(int memory_size) {
        cpu = new PlainCPU(memory_size);
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
        auto _ = fread(buffer, size, 1, f);
    
        printf("load %d bytes at 0x%x\n", size, offset);
        for (int i = 0; i < size; i++) {
            cpu->st8_direct(offset + i, buffer[i]);
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
        uint64_t number = 100000; // a value of 1 enables history recording (slow)
        uint64_t cycles = cpu->cycles + number;
        while (cpu->cycles < cycles) {
            try {
                cpu->fetch_decode_execute(cycles - cpu->cycles);
                if (number == 1 && cycles > history_skip) {
                    history[cpu->cycles % history_size] = compile_status_string();
                }
                if (cpu->halted) {
                    if (number == 1) {
                        for (int i = 0; i < history_size; i++) {
                            std::cout << history[(cpu->cycles + 1 + i) % history_size] << std::endl;
                        }
                    }
                    std::cout << cpu->cycles << std::endl; // tell number of fedex'ed instructions
                    exit(0);
                }
            } catch (const Interrupt& i) {
                int mask = 1 << 6;
                if ((32 > i.id) && (mask & (1 << i.id))) {
                    std::string status = compile_status_string();
                    int n, linear, physical;
                    linear = cpu->segs[1].base + cpu->eip; // EIP is offset of linear address
                    if (cpu->cr0 & 0x80000001) { // protected mode and paging enabled
                        physical = cpu->tlb_lookup(linear, 0); // physical address
                        n = 4096 - (linear & 0xfff); // print bytes left to end-of-page...
                        n = std::min(n, 15); // ...or up to maximum instruction length bytes
                    } else {
                        linear &= 0xfffff;
                        physical = linear;
                        n = 15;
                    }
                    std::string memory = "[EIP..EIP+" + hex((char) (n - 1)) + "]:";
                    for (int i = 0; i < n; i++) {
                        memory += " " + hex((char) cpu->ld8_direct(physical + i));
                    }
                    std::cout
                        << "interrupt id " << i.id
                        << ", error code " << i.error_code
                        << std::endl << status
                        << std::endl << memory << std::endl;
                }
            } catch (const char *m) {
                std::cout << m << std::endl;
                exit(1);
            }
        }
    }
  private:
    PlainCPU *cpu = nullptr;
    std::string compile_status_string(bool compact = true) {
        std::string a, c, d, b, si, di, i, sp, bp, flags;
        std::string cs, ss, ds, es, fs, gs, cr0, cr2, cr3;
        a = "A:" + hex(cpu->regs[0]);
        c = " C:" + hex(cpu->regs[1]);
        d = " D:" + hex(cpu->regs[2]);
        b = " B:" + hex(cpu->regs[3]);
        si = " SI:" + hex(cpu->regs[6]);
        di = " DI:" + hex(cpu->regs[7]);
        i = " I:" + hex((int) cpu->eip);
        sp = " SP:" + hex(cpu->regs[4]);
        bp = " BP:" + hex(cpu->regs[5]);
        flags = " F:" + bin(cpu->eflags, true).substr(22, std::string::npos);
        if (compact) {
            // A:DEADBEAF C:DEADBEAF D:DEADBEAF B:DEADBEAF SI:DEADBEAF DI:DEADBEAF I:CAFE55AA SP:CAFE55AA BP:CAFE55AA F:0001_00001111
            return std::string(a + c + d + b + si + di + i + sp + bp + flags);
        }
        // EAX:00000000                ESP:CAFE55AA
        // ECX:00000000                EBP:CAFE55AA
        // EDX:00000000
        // EBX:00000000
        // ESI:00000000
        // EDI:00000000                EIP:CAFE55AA
        //
        //        EFLAGS:00010001_00010001_00001111
        //
        // CS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // SS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // DS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // ES:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // FS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // GS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        //
        // CR0:0..01111  CR2:DEADBEAF  CR3:DEADB000
        a = "EAX:" + hex(cpu->regs[0]);
        c = "ECX:" + hex(cpu->regs[1]);
        d = "EDX:" + hex(cpu->regs[2]);
        b = "EBX:" + hex(cpu->regs[3]);
        si = "ESI:" + hex(cpu->regs[6]);
        di = "EDI:" + hex(cpu->regs[7]);
        i = "EIP:" + hex((int) cpu->eip);
        sp = "ESP:" + hex(cpu->regs[4]);
        bp = "EBP:" + hex(cpu->regs[5]);
        flags = "EFLAGS:" + bin(cpu->eflags, true).substr(9, std::string::npos);
        cs = "CS:" + hex((short) cpu->segs[1].selector) + ":" + hex((int) cpu->segs[1].base) + ":" + hex((int) cpu->segs[1].limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[1].flags, true);
        ss = "SS:" + hex((short) cpu->segs[2].selector) + ":" + hex((int) cpu->segs[2].base) + ":" + hex((int) cpu->segs[2].limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[2].flags, true);
        ds = "DS:" + hex((short) cpu->segs[3].selector) + ":" + hex((int) cpu->segs[3].base) + ":" + hex((int) cpu->segs[3].limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[3].flags, true);
        es = "ES:" + hex((short) cpu->segs[0].selector) + ":" + hex((int) cpu->segs[0].base) + ":" + hex((int) cpu->segs[0].limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[0].flags, true);
        fs = "FS:" + hex((short) cpu->segs[4].selector) + ":" + hex((int) cpu->segs[4].base) + ":" + hex((int) cpu->segs[4].limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[4].flags, true);
        gs = "GS:" + hex((short) cpu->segs[5].selector) + ":" + hex((int) cpu->segs[5].base) + ":" + hex((int) cpu->segs[5].limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[5].flags, true);
        cr0 = "CR0:" + bin(cpu->cr0).replace(1, 26, "..");
        cr2 = "CR2:" + hex(cpu->cr2);
        cr3 = "CR3:" + hex(cpu->cr3);
        return std::string(
            a + "                " + sp + "\n" +
            c + "                " + bp + "\n" +
            d + "\n" +
            b + "\n" +
            si + "\n" +
            di + "                " + i + "\n" +
            "\n" + "       " + flags + "\n" +
            "\n" + cs + "\n" +
            ss + "\n" +
            ds + "\n" +
            es + "\n" +
            fs + "\n" +
            gs + "\n" +
            "\n" + cr0 + "  " + cr2 + "  " + cr3
        );
    }
    std::string bin(char bits) {
    #define V(byte) \
        ((byte) & 80 ? '1' : '0') + \
        ((byte) & 40 ? '1' : '0') + \
        ((byte) & 20 ? '1' : '0') + \
        ((byte) & 10 ? '1' : '0') + \
        ((byte) &  8 ? '1' : '0') + \
        ((byte) &  4 ? '1' : '0') + \
        ((byte) &  2 ? '1' : '0') + \
        ((byte) &  1 ? '1' : '0')
        std::string result = "";
        return result + V(bits);
    }
    std::string bin(short bits, bool divide = false) {
        return bin((char)(bits >> 8)) + (divide ? "_" : "") + bin((char)(bits & 0xff));
    }
    std::string bin(int bits, bool divide = false) {
        return bin((short)(bits >> 16), divide) + (divide ? "_" : "") + bin((short)(bits & 0xffff), divide);
    }
    std::string hex(char bits) {
        std::string result = ""; char numerals[] = "0123456789ABCDEF";
        return result + numerals[((bits >> 4) & 0xf)] + numerals[bits & 0xf];
    }
    std::string hex(short bits) {
        return hex((char)(bits >> 8)) + hex((char)(bits & 0xff));
    }
    std::string hex(int bits) {
        return hex((short)(bits >> 16)) + hex((short)(bits & 0xffff));
    }
/*
 Enable history recording in case the test suite unexpectedly terminates
 before its scheduled end (both by executing hlt). Doing so, first run
 the suite without recording. The very last number in the output states
 the number of instructions executed. Reduce it by, for example, 10000,
 and set `history_skip' to the result.
 This will start recording only shortly before the termination, saving
 you some time.
 */
    static const int history_skip = 0;
    static const int history_size = 512;
    std::string history[history_size];
};

int PlainCPU::io_read(int port) {
    int _port = port & (1024 - 1);
    printf("*** ioport_read 0x%04x\n", _port);
    return 0xff;
}
void PlainCPU::io_write(int port, int data) {
    int _port = port & (1024 - 1);
    if (_port == 0x0190) { // default POST_PORT in test386
        printf("*** ioport_write 0x%04x : 0x%08x\n", _port, data);
    } else { // any other value considered OUT_PORT
        printf("%c", data);
    }
}

#endif // TEST386_H
