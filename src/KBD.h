#ifndef _H_KBD
#define _H_KBD

#include <fstream>

class KBD {

  public:
    KBD()
    {
    }

    int read_status(int mem8_loc)
    {
        return 0;
    }

    void write_command(int mem8_loc, int x)
    {
        switch (x) {
            case 0xfe: // Resend command (PC AT Technical Reference, sec. 4-1)
                break;
            default:
                break;
        }
    }
};

#endif // _H_KBD