#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>

#define I2C_BUS "/dev/i2c-1"
#define RTC_ADDR 0x68

uint8_t dec_to_bcd(int val)
{
    return ((val / 10) << 4) | (val % 10);
}

int bcd_to_dec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}

int main(void)
{
    int fd;
    uint8_t buf[8];
    time_t now;
    struct tm *t;

    fd = open(I2C_BUS, O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, RTC_ADDR) < 0)
    {
        perror("ioctl");
        close(fd);
        return -1;
    }

    /* Get current system time */
    now = time(NULL);
    t = localtime(&now);

    /* Prepare RTC registers */
    buf[0] = 0x00;                          // Start register
    buf[1] = dec_to_bcd(t->tm_sec);
    buf[2] = dec_to_bcd(t->tm_min);
    buf[3] = dec_to_bcd(t->tm_hour);
    buf[4] = dec_to_bcd(t->tm_wday == 0 ? 7 : t->tm_wday);
    buf[5] = dec_to_bcd(t->tm_mday);
    buf[6] = dec_to_bcd(t->tm_mon + 1);
    buf[7] = dec_to_bcd(t->tm_year % 100);

    /* Write current time to RTC */
    if (write(fd, buf, 8) != 8)
    {
        perror("RTC write");
        close(fd);
        return -1;
    }

    printf("RTC updated successfully.\n\n");

    while (1)
    {
        uint8_t reg = 0x00;
        uint8_t data[7];

        write(fd, &reg, 1);

        if (read(fd, data, 7) != 7)
        {
            perror("RTC read");
            break;
        }

        printf("\rTime: %02d:%02d:%02d   Date: %02d/%02d/20%02d",
               bcd_to_dec(data[2] & 0x3F),
               bcd_to_dec(data[1]),
               bcd_to_dec(data[0] & 0x7F),
               bcd_to_dec(data[4]),
               bcd_to_dec(data[5]),
               bcd_to_dec(data[6]));

        fflush(stdout);
        sleep(1);
    }

    close(fd);
    return 0;
}
