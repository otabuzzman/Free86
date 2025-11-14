#ifndef _H_PC
#define _H_PC

#include <cstddef>
#include <stdexcept>
#include <vector>

#ifndef NO_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

#include "x86/x86.h"

class PC {
  public:
    PC(int mem_size);
    ~PC();

    int  load(std::string path, int offset = 0);
    void setup();
    void cycle();

#ifndef NO_SDL
    void paint(SDL_Renderer *render, int widht, int height);
#else
    void print();
    void input();
#endif

  private:
    x86Internal *cpu  = nullptr;
#ifndef NO_SDL
    TTF_Font    *font = nullptr;
#endif
};

#endif // _H_PC
