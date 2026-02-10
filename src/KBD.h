#ifndef KBD_H
#define KBD_H

#include <fstream>

class KBD {

  public:
    KBD()
    {
    }

    int read_status(int port)
    {
        return 0;
    }

    void write_command(int port, int data)
    {
        switch (data) {
            case 0xfe: // Resend command (PC AT Technical Reference, sec. 4-1)
                break;
            default:
                break;
        }
    }
};

#endif // KBD_H
