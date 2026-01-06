#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_DEVICE "/dev/i2c-2"
#define I2C_ADDR   0x50

int main(void) {
    int f, i = 0, n = 0;
    char buf[10];

    // 1. Open the I2C Bus
    if ((f = open(I2C_DEVICE, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        return 1;
    }

    // 2. Set the I2C Slave Address
    if (ioctl(f, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        return 1;
    }

    // --- WRITE OPERATION ---
    // buf[0] is the internal memory address (0x50) where writing starts
    buf[0] = 0x50; 
    
    // Fill buffer with data to write (0x30 to 0x38)
    for(i = 1; i < 10; i++) {
        buf[i] = 0x30 + (i - 1); 
    }

    // Write the buffer (1 address byte + 9 data bytes)
    if (write(f, buf, 10) != 10) {
        perror("Failed to write to the i2c bus");
    } else {
        printf("Written 9 bytes to EEPROM starting at 0x50\n");
    }

    // Wait for write cycle to complete
    usleep(10000); 

    // --- READ OPERATION ---
    
    // 1. Write the address we want to read from (dummy write)
    buf[0] = 0x50;
    if (write(f, buf, 1) != 1) {
        perror("Failed to set read address");
    }

    // 2. Clear buffer before reading
    for (i = 0; i < 10; i++) buf[i] = 0;

    // 3. Read 9 bytes back
    if (read(f, buf, 9) != 9) {
        perror("Failed to read from the i2c bus");
    } else {
        printf("Read Data:\n");
        for(i = 0; i < 9; i++) {
            printf("0x%02x ", buf[i]);
        }
        printf("\n");
    }

    close(f);
    return 0;
}