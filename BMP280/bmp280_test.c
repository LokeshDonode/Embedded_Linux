#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define I2C_BUS "/dev/i2c-1"
#define BMP280_ADDR 0x77

int main() {
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, BMP280_ADDR) < 0) {
        perror("Failed to select I2C device");
        close(fd);
        return 1;
    }

    // Read chip ID (0xD0)
    unsigned char reg = 0xD0;
    write(fd, &reg, 1);

    unsigned char data;
    read(fd, &data, 1);

    printf("BME280 Chip ID: 0x%X\n", data);  // Should print 0x60

    close(fd);
    return 0;
}
