#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define RTC_ADDR 0x68

int bcd_to_dec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}

const char *days[] =
{
    "",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat",
    "Sun"
};

int days_in_month(int month, int year)
{
    switch(month)
    {
        case 1: return 31;
        case 2:
            if((year%4==0 && year%100!=0) || (year%400==0))
                return 29;
            else
                return 28;

        case 3: return 31;
        case 4: return 30;
        case 5: return 31;
        case 6: return 30;
        case 7: return 31;
        case 8: return 31;
        case 9: return 30;
        case 10:return 31;
        case 11:return 30;
        case 12:return 31;
    }

    return 30;
}

int main()
{
    int fd;

    fd = open(I2C_BUS, O_RDWR);
    if(fd < 0)
    {
        perror("open");
        return -1;
    }

    if(ioctl(fd, I2C_SLAVE, RTC_ADDR) < 0)
    {
        perror("ioctl");
        return -1;
    }

    while(1)
    {
        uint8_t reg = 0x00;
        uint8_t rtc[7];

        if(write(fd,&reg,1)!=1)
        {
            perror("write");
            break;
        }

        if(read(fd,rtc,7)!=7)
        {
            perror("read");
            break;
        }

        int sec  = bcd_to_dec(rtc[0] & 0x7F);
        int min  = bcd_to_dec(rtc[1]);

        int hour;

        if(rtc[2] & 0x40)
        {
            /* 12-hour mode */
            hour = bcd_to_dec(rtc[2] & 0x1F);

            if(rtc[2] & 0x20)
            {
                if(hour != 12)
                    hour += 12;
            }
            else
            {
                if(hour == 12)
                    hour = 0;
            }
        }
        else
        {
            /* 24-hour mode */
            hour = bcd_to_dec(rtc[2] & 0x3F);
        }

        int day   = bcd_to_dec(rtc[3]);
        int date  = bcd_to_dec(rtc[4]);
        int month = bcd_to_dec(rtc[5]);
        int year  = 2000 + bcd_to_dec(rtc[6]);

        /******** UTC -> IST ********/

        min += 30;

        if(min >= 60)
        {
            min -= 60;
            hour++;
        }

        hour += 5;

        if(hour >= 24)
        {
            hour -= 24;

            day++;
            if(day > 7)
                day = 1;

            date++;

            if(date > days_in_month(month,year))
            {
                date = 1;
                month++;

                if(month > 12)
                {
                    month = 1;
                    year++;
                }
            }
        }

        /******** Temperature ********/

        uint8_t treg = 0x11;
        uint8_t temp[2];

        if(write(fd,&treg,1)!=1)
        {
            perror("temp write");
            break;
        }

        if(read(fd,temp,2)!=2)
        {
            perror("temp read");
            break;
        }

        float temperature =
            (int8_t)temp[0] +
            ((temp[1] >> 6) * 0.25);

        printf("\r%s  %02d/%02d/%04d  %02d:%02d:%02d  Temp: %.2f C",
               days[day],
               date,
               month,
               year,
               hour,
               min,
               sec,
               temperature);

        fflush(stdout);

        sleep(1);
    }

    close(fd);

    return 0;
}
