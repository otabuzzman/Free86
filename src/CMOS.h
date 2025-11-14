#ifndef _H_CMOS
#define _H_CMOS

#include <fstream>
#include <time.h>

class CMOS {
    uint8_t bytes[128]{0};
    int     index = 0;

  public:
    CMOS()
    {
        bytes[10]   = 0x26; // RTC status register A: 32768 Hz time base, 976562 msec INT rate
        bytes[11]   = 0x02; // RTC status register B: 24 hours
        bytes[12]   = 0x00; // RTC status register C
        bytes[13]   = 0x80; // RTC status register D: battery power good
        bytes[0x14] = 0x02; // IBM equipment byte: math coprocessor installed
    }

    void ioport_write(int mem8_loc, int data)
    {
        if (mem8_loc == 0x70) {
            index = data & 0x7f;
        }
    }

    int ioport_read(int mem8_loc)
    {
        int data;
        time_t clock;
        struct tm *utc;

        if (mem8_loc == 0x70) {
            return 0xff;
        }

        time(&clock);
        utc = gmtime(&clock);

        bytes[0] = bin_to_bcd(utc->tm_sec);
        bytes[2] = bin_to_bcd(utc->tm_min);
        bytes[4] = bin_to_bcd(utc->tm_hour);
        bytes[6] = bin_to_bcd(utc->tm_wday);
        bytes[7] = bin_to_bcd(utc->tm_mday);
        bytes[8] = bin_to_bcd(utc->tm_mon + 1);
        bytes[9] = bin_to_bcd(utc->tm_year);

        data = bytes[index];
        if (index == 10) {
            bytes[10] ^= 0x80; // XOR emulates data update cycle
        }
        return data;
    }

  private:
    int bin_to_bcd(int a)
    {
        return ((a / 10) << 4) | (a % 10);
    }
};

#endif // _H_CMOS
