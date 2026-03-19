#include <stdio.h>   
#include <fcntl.h>     
#include <unistd.h>    
#include <linux/i2c-dev.h> 
#include <sys/ioctl.h>  

#define DS3231_ADDR 0x68   // I2C address of DS3231 RTC
// Function: Convert BCD (Binary Coded Decimal) to normal decimal
int bcd_to_dec(unsigned char val) {
    // Example: 0x25 → (2 * 10) + 5 = 25
    return (val / 16 * 10) + (val % 16);
}

int main() {
    // Step 1: Declare variables
    int file;   // File descriptor for I2C device
    char *filename = "/dev/i2c-1";  // I2C bus on Raspberry Pi
  
    // Step 2: Open I2C bus
    file = open(filename, O_RDWR);
    if (file < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    // Step 3: Select RTC device using its I2C address
    if (ioctl(file, I2C_SLAVE, DS3231_ADDR) < 0) {
        perror("Failed to connect to RTC");
        return 1;
    }

    // Step 4: Tell RTC where to start reading
    unsigned char reg = 0x00; // Address of seconds register

    write(file, &reg, 1); // This sets internal pointer of RTC to register 0x00

    // Step 5: Read 7 bytes from RTC
    unsigned char data[7];

    read(file, data, 7);
    /*
        data[0] → seconds
        data[1] → minutes
        data[2] → hours
        data[3] → day (not used here)
        data[4] → date
        data[5] → month
        data[6] → year
    */

    // Step 6: Convert BCD values to decimal
    int sec   = bcd_to_dec(data[0]);
    int min   = bcd_to_dec(data[1]);
    int hour  = bcd_to_dec(data[2]);
    int date  = bcd_to_dec(data[4]);
    int month = bcd_to_dec(data[5]);
    int year  = bcd_to_dec(data[6]) + 2000;
    
    // DS3231 stores year as last 2 digits → add 2000

    // Step 7: Print formatted output
    printf("Date: %02d-%02d-%04d\n", date, month, year);
    printf("Time: %02d:%02d:%02d\n", hour, min, sec);

    // Step 8: Close I2C device
    close(file);

    return 0;
}
