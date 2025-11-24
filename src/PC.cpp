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

PC::PC(x86Internal cpu, int mem_size)
{
    this->cpu = cpu;
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
}

int PC::load(std::string path, int offset)
{
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

void PC::setup()
{
    load("bin/linuxstart.bin",             0x00010000); // custom bootloader
    load("bin/vmlinux-2.6.20.bin",         0x00100000); // Linux kernel
    int initrd_size = load("bin/root.bin", 0x00400000); // initial ramdisk (root fs)

    int cmdline_addr = 0x0f800;
    std::string cmdline = "console=ttyS0 root=/dev/ram0 rw init=/sbin/init notsc=1";

    cpu->st8_phys(cmdline_addr, cmdline);
    printf("%s\n", cmdline.c_str());

    cpu->eip = 0x10000;
    cpu->segs[1].flags = (1 << 22); // CS, Bit 22 = 1 for 32bit segment
    cpu->segs[2].flags = (1 << 22); // SS, Bit 22 = 1 for 32bit segment
    cpu->segs[3].flags = (1 << 9);  // DS, Bit 9 = writable
    cpu->cr0 = (1 << 0);  // PE-mode ON

    cpu->regs[0] = 16 * 1024 * 1024;
    cpu->regs[1] = cmdline_addr;
    cpu->regs[3] = initrd_size;

    printf("\n\n************************\n");
    printf("************************\n");
    printf("****** Boot Linux ******\n");
    printf("************************\n");
    printf("************************\n\n\n");
}

void PC::cycle()
{
    int cycles_requested = cpu->cycles_processed + 100000;

    while (cpu->cycles_processed < cycles_requested) {
        cpu->pit->update_irq();

        try {
            cpu->fetch_decode_execute(cycles_requested - cpu->cycles_processed);
            if (cpu->halted)
                break;
        } catch (Interrupt) {}
    }
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
int WiredCPU::get_hard_irq() {
    return pic->irq;
}
int WiredCPU::get_hard_intno() {
    return pic->get_hard_intno();
}
int WiredCPU::ioport_read(int mem8_loc) {
    int port = mem8_loc & (1024 - 1);
    switch (port) {
    case 0x70:
    case 0x71:
        return cmos->ioport_read(mem8_loc);
    case 0x64:
        return kbd->read_status(mem8_loc);
    case 0x20:
    case 0x21:
        return pic->pics[0]->ioport_read(mem8_loc);
    case 0xa0:
    case 0xa1:
        return pic->pics[1]->ioport_read(mem8_loc);
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
        return pit->ioport_read(mem8_loc);
    case 0x61:
        return pit->speaker_ioport_read(mem8_loc);
    case 0x3f8:
    case 0x3f9:
    case 0x3fa:
    case 0x3fb:
    case 0x3fc:
    case 0x3fd:
    case 0x3fe:
    case 0x3ff:
        return serial->ioport_read(mem8_loc);
    default:
        return 0xff;
    }
}
void WiredCPU::ioport_write(int mem8_loc, int data) {
    int port = mem8_loc & (1024 - 1);
    switch (port) {
    case 0x80: // POST
        break;
    case 0x70:
    case 0x71:
        cmos->ioport_write(mem8_loc, data);
        break;
    case 0x64:
        kbd->write_command(mem8_loc, data);
        break;
    case 0x20:
    case 0x21:
        try {
            pic->pics[0]->ioport_write(mem8_loc, data);
        } catch (const char *) {
        }
        break;
    case 0xa0:
    case 0xa1:
        try {
            pic->pics[1]->ioport_write(mem8_loc, data);
        } catch (const char *) {
        }
        break;
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
        pit->ioport_write(mem8_loc, data);
        break;
    case 0x61:
        pit->speaker_ioport_write(mem8_loc, data);
        break;
    case 0x3f8:
    case 0x3f9:
    case 0x3fa:
    case 0x3fb:
    case 0x3fc:
    case 0x3fd:
    case 0x3fe:
    case 0x3ff:
        serial->ioport_write(mem8_loc, data);
        break;
    }
}
