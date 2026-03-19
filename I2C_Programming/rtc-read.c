#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define DS3231_ADDR 0x68

// Convert BCD to Decimal
int bcd_to_dec(unsigned char val) {
    return (val / 16 * 10) + (val % 16);
}

int main() {

    int file;
    char *filename = "/dev/i2c-1";

    // Open I2C
    file = open(filename, O_RDWR);
    if (file < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    // Connect to RTC
    if (ioctl(file, I2C_SLAVE, DS3231_ADDR) < 0) {
        perror("Failed to connect to RTC");
        return 1;
    }

    printf("Logging RTC Time & Temperature (every 1 second)\n\n");

    while (1) {

        //--------------------------------------------------
        // Step 1: Read Time (Registers 0x00 → 0x06)
        //--------------------------------------------------
        unsigned char reg = 0x00;
        write(file, &reg, 1);

        unsigned char data[7];
        read(file, data, 7);

        int sec   = bcd_to_dec(data[0]);
        int min   = bcd_to_dec(data[1]);
        int hour  = bcd_to_dec(data[2]);
        int date  = bcd_to_dec(data[4]);
        int month = bcd_to_dec(data[5]);
        int year  = bcd_to_dec(data[6]) + 2000;

        //--------------------------------------------------
        // Step 2: Read Temperature (Registers 0x11, 0x12)
        //--------------------------------------------------
        unsigned char temp_reg = 0x11;
        write(file, &temp_reg, 1);

        unsigned char temp_data[2];
        read(file, temp_data, 2);

        int temp_msb = temp_data[0];
        int temp_lsb = temp_data[1];

        float temperature = temp_msb + ((temp_lsb >> 6) * 0.25);

        //--------------------------------------------------
        // Step 3: Print Output
        //--------------------------------------------------
        printf("Date: %02d-%02d-%04d | Time: %02d:%02d:%02d | Temp: %.2f°C\n",
               date, month, year, hour, min, sec, temperature);

        //--------------------------------------------------
        // Step 4: Delay (1 second)
        //--------------------------------------------------
        sleep(1);
    }

    close(file);
    return 0;
}
