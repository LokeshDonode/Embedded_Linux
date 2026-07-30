#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"  // I2C device on Raspberry Pi (bus 1)
#define RTC_ADDR 0x68          // Typical I2C address for DS1307/DS3231 RTCs

// Convert a BCD (binary-coded decimal) value from the RTC to a normal integer
int bcd_to_dec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}

// Short names for weekday printing. Index corresponds to RTC weekday register (1..7).
const char *days[] =
{
    "",   // index 0 not used by RTC (RTC stores 1..7)
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat",
    "Sun"
};

// Return the number of days in the given month/year (handles leap years for February)
int days_in_month(int month, int year)
{
    switch(month)
    {
        case 1: return 31;
        case 2:
            // Leap year rules: divisible by 4 and not by 100, or divisible by 400
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

    // Fallback (shouldn't happen for valid months)
    return 30;
}

int main()
{
    int fd;

    // Open the I2C device file (requires appropriate permissions or use sudo)
    fd = open(I2C_BUS, O_RDWR);
    if(fd < 0)
    {
        perror("open");
        return -1;
    }

    // Tell the I2C driver which device address we want to talk to
    if(ioctl(fd, I2C_SLAVE, RTC_ADDR) < 0)
    {
        perror("ioctl");
        return -1;
    }

    // Main loop: repeatedly read RTC registers and temperature every second
    while(1)
    {
        uint8_t reg = 0x00;   // Start reading from register 0
        uint8_t rtc[7];       // DS3231/DS1307: seconds, minutes, hours, day, date, month, year

        // Set the register pointer in the RTC to 0x00 by writing a single byte (the register)
        if(write(fd,&reg,1)!=1)
        {
            perror("write");
            break;
        }

        // Read 7 bytes of time/date data
        if(read(fd,rtc,7)!=7)
        {
            perror("read");
            break;
        }

        // Decode BCD-encoded time values from the RTC registers
        int sec  = bcd_to_dec(rtc[0] & 0x7F); // bit7 is CH (clock halt) for some RTCs
        int min  = bcd_to_dec(rtc[1]);

        int hour;

        // Hour register may be in 12-hour or 24-hour mode. Check the 6th bit (0x40)
        if(rtc[2] & 0x40)
        {
            /* 12-hour mode */
            hour = bcd_to_dec(rtc[2] & 0x1F);

            // In 12-hour mode, bit 5 (0x20) indicates PM when set
            if(rtc[2] & 0x20)
            {
                if(hour != 12)
                    hour += 12; // convert PM to 24-hour representation
            }
            else
            {
                // 12 AM hour is encoded as 12 -> convert to 0
                if(hour == 12)
                    hour = 0;
            }
        }
        else
        {
            /* 24-hour mode */
            hour = bcd_to_dec(rtc[2] & 0x3F);
        }

        int day   = bcd_to_dec(rtc[3]);  // Day of week (1 = Monday .. 7 = Sunday expected in this code's days[])
        int date  = bcd_to_dec(rtc[4]);  // Day of month
        int month = bcd_to_dec(rtc[5]);  // Month (1-12)
        int year  = 2000 + bcd_to_dec(rtc[6]); // Year since 2000

        /******** UTC -> IST conversion ********/
         * The RTC typically holds UTC. This code converts UTC to IST (UTC+5:30).
         * It adds 30 minutes and then 5 hours, handling day/month/year rollovers.
         */

        min += 30; // add 30 minutes for IST

        if(min >= 60)
        {
            min -= 60;
            hour++;
        }

        hour += 5; // add 5 hours for IST

        if(hour >= 24)
        {
            // roll over to next day
            hour -= 24;

            day++;
            if(day > 7)
                day = 1;

            date++;

            // handle month/year rollover
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

        /******** Temperature read (DS3231 provides temperature at register 0x11 & 0x12) ********/

        uint8_t treg = 0x11; // temperature MSB register for DS3231
        uint8_t temp[2];

        // Point to the temperature register and read two bytes
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

        // Temperature is stored as: temp[0] = signed int8 MSB, temp[1] top two bits = fractional part (0.25 increments)
        float temperature =
            (int8_t)temp[0] +
            ((temp[1] >> 6) * 0.25);

        // Print a single-line status updating in place (carriage return at start)
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

        sleep(1); // wait 1 second before next read
    }

    close(fd);

    return 0;
}
