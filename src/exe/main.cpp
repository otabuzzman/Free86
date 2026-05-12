#include <iostream>
#include <signal.h>
#include <thread>

#ifndef NO_SDL
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

#include "PC.h"

#ifdef TEST386
#include "Test386.h"
#endif

int Running = 1;

#ifndef NO_SDL
void render_loop(PC *pc, SDL_Renderer *render, int width, int height)
{
    while (Running) {
        pc->paint(render, width, height);
    }
}
#else
void on_signal(int sigdef)
{
    Running = 0; // threads will most likely block in queues
    std::cout << std::flush;
    std::cerr << std::flush;
    exit(0);
}
#ifndef TEST386
void print_loop(PC *pc)
{
    while (Running) {
        pc->print();
    }
}
void input_loop(PC *pc)
{
    while (Running) {
        pc->input();
    }
}
#endif // TEST386
#endif // NO_SDL
int main(int ArgCount, char **Args)
{
    static const int width = 840, height = 350;
    static const uint32_t memory_size = 0x01000000; // 16 MB
#ifdef TEST386
    Test386 *pc = new Test386(memory_size);
#else
    PC *pc = new PC(memory_size);
#endif
    signal(SIGINT, on_signal);

    pc->setup();

    int speed = 20;

#ifndef NO_SDL
    SDL_Window *window =
        SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(render, 1, 1);

    std::thread th(render_loop, pc, render, width, height);
#else
#ifndef TEST386
    std::thread print(print_loop, pc);
    std::thread input(input_loop, pc);
#endif // TEST386
#endif // NO_SDL

    while (Running) {
        pc->cycle();
#ifndef NO_SDL
        SDL_Event Event;
        while (SDL_PollEvent(&Event)) {
            if (Event.type == SDL_QUIT)
                Running = 0;
        }
#endif
#ifndef NO_SDL
        if (speed != 100)
            SDL_Delay(1000 / speed);
#else
#ifndef TEST386
        if (speed != 100)
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / speed));
#endif // TEST386
#endif // NO_SDL
    }

#if !defined(NO_SDL) && !defined(TEST386)
    th.join();
#endif
    return 0;
}
