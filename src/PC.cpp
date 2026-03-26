#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>

#ifndef NO_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

#include "PC.h"

PC::PC(uint32_t memory_size)
{
    this->cpu = new WiredCPU(memory_size);
#ifndef NO_SDL
    TTF_Init();
    font = TTF_OpenFont("bin/cp437.ttf", 14);
    if (font == NULL) {
        printf("error: font not found\n");
        exit(EXIT_FAILURE);
    }
#endif
}

PC::~PC()
{
    delete cpu;
}

size_t PC::load(std::string path, uint32_t offset)
{
    FILE *f = fopen(path.c_str(), "rb");
    fseek(f, 0, SEEK_END);
    size_t size = static_cast<size_t>(ftell(f));
    fseek(f, 0, SEEK_SET);
    auto buffer = new uint8_t[size];
    auto _ = fread(buffer, size, 1, f);

    printf("load %lu bytes at 0x%x\n", size, offset);
    for (size_t i = 0; i < size; i++) {
        cpu->st8_direct(offset + i, buffer[i]);
    }
    fclose(f);

    return size;
}

void PC::setup()
{
    load("bin/linuxstart.bin",     0x00010000); // custom bootloader
    load("bin/vmlinux-2.6.20.bin", 0x00100000); // Linux kernel
    size_t initrd_size = load("bin/root.bin", 0x00400000); // initial ramdisk (root fs)

    uint32_t cmdline_addr = 0x0f800;
    std::string cmdline = "console=ttyS0 root=/dev/ram0 rw init=/sbin/init notsc=1";

    // set memory state
    cpu->st8_direct(cmdline_addr, cmdline);
    printf("%s\n", cmdline.c_str());
    // end of set memory state

    // set CPU state (see bin/bootstrap.asm)
    load("bin/bootstrap.bin", 0x000f0000);
    // end of set CPU state

    printf("\n\n************************\n");
    printf("************************\n");
    printf("****** Boot Linux ******\n");
    printf("************************\n");
    printf("************************\n\n\n");
}

void PC::cycle()
{
    uint64_t cycles = cpu->cycles + 100000;
    Interrupt interrupt = {-1, 0};

    while (cpu->cycles < cycles) {
        cpu->pit->update_irq();

        try {
            cpu->fetch_decode_execute(cycles - cpu->cycles, interrupt);
            if (cpu->halted)
                break;
        } catch (const Interrupt& e) {
            interrupt = {e.id, e.error_code};
        } catch (const char *m) {
            std::cout << m << std::endl;
            exit(1);
        }
    }
}
std::string PC::state() {
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
    std::string a, c, d, b, si, di, i, sp, bp, flags;
    std::string cs, ss, ds, es, fs, gs, cr0, cr2, cr3;
    a = "EAX:" + hex(cpu->regs[0]);
    c = "ECX:" + hex(cpu->regs[1]);
    d = "EDX:" + hex(cpu->regs[2]);
    b = "EBX:" + hex(cpu->regs[3]);
    si = "ESI:" + hex(cpu->regs[6]);
    di = "EDI:" + hex(cpu->regs[7]);
    i = "EIP:" + hex((int) cpu->eip);
    sp = "ESP:" + hex(cpu->regs[4]);
    bp = "EBP:" + hex(cpu->regs[5]);
    flags = "EFLAGS:" + bin(cpu->eflags, true).substr(9);
    cs = "CS:" + hex((short) cpu->segs[1].selector) + ":" + hex(cpu->segs[1].shadow.base) + ":" + hex(cpu->segs[1].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[1].shadow.flags, true);
    ss = "SS:" + hex((short) cpu->segs[2].selector) + ":" + hex(cpu->segs[2].shadow.base) + ":" + hex(cpu->segs[2].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[2].shadow.flags, true);
    ds = "DS:" + hex((short) cpu->segs[3].selector) + ":" + hex(cpu->segs[3].shadow.base) + ":" + hex(cpu->segs[3].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[3].shadow.flags, true);
    es = "ES:" + hex((short) cpu->segs[0].selector) + ":" + hex(cpu->segs[0].shadow.base) + ":" + hex(cpu->segs[0].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[0].shadow.flags, true);
    fs = "FS:" + hex((short) cpu->segs[4].selector) + ":" + hex(cpu->segs[4].shadow.base) + ":" + hex(cpu->segs[4].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[4].shadow.flags, true);
    gs = "GS:" + hex((short) cpu->segs[5].selector) + ":" + hex(cpu->segs[5].shadow.base) + ":" + hex(cpu->segs[5].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[5].shadow.flags, true);
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
std::string PC::compact_state() {
    // A:DEADBEAF C:DEADBEAF D:DEADBEAF B:DEADBEAF SI:DEADBEAF DI:DEADBEAF I:CAFE55AA SP:CAFE55AA BP:CAFE55AA F:0001_00001111
    std::string a, c, d, b, si, di, i, sp, bp, flags;
    a = "A:" + hex(cpu->regs[0]);
    c = " C:" + hex(cpu->regs[1]);
    d = " D:" + hex(cpu->regs[2]);
    b = " B:" + hex(cpu->regs[3]);
    si = " SI:" + hex(cpu->regs[6]);
    di = " DI:" + hex(cpu->regs[7]);
    i = " I:" + hex((int) cpu->eip);
    sp = " SP:" + hex(cpu->regs[4]);
    bp = " BP:" + hex(cpu->regs[5]);
    flags = " F:" + bin(cpu->eflags, true).substr(22);
    return std::string(a + c + d + b + si + di + i + sp + bp + flags);
}
std::string PC::eip_15() {
    uint32_t linear, physical;
    int n;
    linear = cpu->segs[1].shadow.base + cpu->eip; // EIP is offset of linear address
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
    return memory;
}
std::string PC::bin(char bits) {
#define V(byte) \
    ((byte) & 0x80 ? "1" : "0") + \
    ((byte) & 0x40 ? "1" : "0") + \
    ((byte) & 0x20 ? "1" : "0") + \
    ((byte) & 0x10 ? "1" : "0") + \
    ((byte) &  8 ? "1" : "0") + \
    ((byte) &  4 ? "1" : "0") + \
    ((byte) &  2 ? "1" : "0") + \
    ((byte) &  1 ? "1" : "0")
    std::string result = "";
    return result + V(bits);
}
std::string PC::bin(short bits, bool divide) {
    return bin((char)(bits >> 8)) + (divide ? "_" : "") + bin((char)(bits & 0xff));
}
std::string PC::bin(int bits, bool divide) {
    return bin((short)(bits >> 16), divide) + (divide ? "_" : "") + bin((short)(bits & 0xffff), divide);
}
std::string PC::bin(uint32_t bits, bool divide) {
    return bin((short)(bits >> 16), divide) + (divide ? "_" : "") + bin((short)(bits & 0xffff), divide);
}
std::string PC::hex(char bits) {
    std::string result = ""; char numerals[] = "0123456789ABCDEF";
    return result + numerals[((bits >> 4) & 0xf)] + numerals[bits & 0xf];
}
std::string PC::hex(short bits) {
    return hex((char)(bits >> 8)) + hex((char)(bits & 0xff));
}
std::string PC::hex(int bits) {
    return hex((short)(bits >> 16)) + hex((short)(bits & 0xffff));
}
std::string PC::hex(uint32_t bits) {
    return hex((short)(bits >> 16)) + hex((short)(bits & 0xffff));
}

#ifndef NO_SDL
void PC::paint(SDL_Renderer *renderer, int widht, int height)
{
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 11, 11, 11, SDL_ALPHA_OPAQUE);
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = 560;
    rect.h = 300;

    SDL_RenderFillRect(renderer, &rect);
    int       stridx = cpu->serial->strbufs_idx;
    SDL_Color color  = {50, 205, 50};

    for (int y = 0; y < 25; ++y) {
        std::string str = "";

        if (24 < stridx) {
            str = cpu->serial->strbufs[stridx - 24 + y];
        } else {
            str = cpu->serial->strbufs[y];
        }
        int strlen = str.length();
        if (strlen == 0)
            continue;

        SDL_Rect r1;
        r1.x = 0;
        r1.y = y * 14;
        r1.w = strlen * 9;
        r1.h = 14;

        SDL_Surface *text_surface = TTF_RenderUTF8_Solid(font, str.c_str(), color);
        SDL_Texture *Message      = SDL_CreateTextureFromSurface(renderer, text_surface);

        SDL_RenderCopy(renderer, Message, NULL, &r1);
        SDL_FreeSurface(text_surface);
        SDL_DestroyTexture(Message);
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(10);
}
#else
void PC::print()
{
    char chr;
    chr = cpu->serial->print_fifo_pop();

    std::cout << chr << std::flush;
}

void PC::input()
{
    int chr;
    chr = std::cin.get();

    cpu->serial->input_fifo_push(chr);
}
#endif // NO_SDL
int WiredCPU::get_irq() {
    return pic->irq;
}
int WiredCPU::get_iid() {
    return pic->read_irq();
}
uint32_t WiredCPU::io_read(uint32_t port) {
    int _port = static_cast<int>(port);
    int data;
    switch (_port) {
    case 0x70:
    case 0x71:
        data = cmos->ioport_read(_port);
        break;
    case 0x20:
    case 0x21:
        data = pic->pics[0]->ioport_read(_port);
        break;
    case 0xa0:
    case 0xa1:
        data = pic->pics[1]->ioport_read(_port);
        break;
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
        data = pit->ioport_read(_port);
        break;
    case 0x61:
        data = pit->speaker_ioport_read(_port);
        break;
    case 0x3f8:
    case 0x3f9:
    case 0x3fa:
    case 0x3fb:
    case 0x3fc:
    case 0x3fd:
    case 0x3fe:
    case 0x3ff:
        data = serial->ioport_read(_port);
        break;
    default:
        data = 0xff;
        break;
    }
    return static_cast<uint32_t>(data);
}
void WiredCPU::io_write(uint32_t port, uint32_t data) {
    int _port = static_cast<int>(port);
    int _data = static_cast<int>(data);
    switch (_port) {
    case 0x80: // POST
        break;
    case 0x70:
    case 0x71:
        cmos->ioport_write(_port, _data);
        break;
    case 0x20:
    case 0x21:
        try {
            pic->pics[0]->ioport_write(_port, _data);
        } catch (const char *) {
        }
        break;
    case 0xa0:
    case 0xa1:
        try {
            pic->pics[1]->ioport_write(_port, _data);
        } catch (const char *) {
        }
        break;
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
        pit->ioport_write(_port, _data);
        break;
    case 0x61:
        pit->speaker_ioport_write(_port, _data);
        break;
    case 0x3f8:
    case 0x3f9:
    case 0x3fa:
    case 0x3fb:
    case 0x3fc:
    case 0x3fd:
    case 0x3fe:
    case 0x3ff:
        serial->ioport_write(_port, _data);
        break;
    }
}
