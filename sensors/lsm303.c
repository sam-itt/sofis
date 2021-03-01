#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include "lsm303.h"

Lsm303 *lsm303_init(Lsm303 *self, const char *bus)
{
    memset(self, 0, sizeof(Lsm303));

    self->fd = open(bus, O_RDWR);
    if(self->fd < 0)
        return NULL;

    return self;
}

Lsm303 *lsm303_dispose(Lsm303 *self)
{
    if(self->fd >= 0)
        close(self->fd);
    return self;
}

void lsm303_start_accelerometer(Lsm303 *self)
{
    // Get I2C device, LSM303DLHC ACCELERO I2C address is 0x19(25)
    ioctl(self->fd, I2C_SLAVE, 0x19);
    // Select control register1(0x20)
    // X, Y and Z-axis enable, power on mode, o/p data rate 10 Hz(0x27)
    char config[2] = {0};
    config[0] = 0x20;
    config[1] = 0x27;
    write(self->fd, config, 2);
    // Select control register4(0x23)
    // Full scale +/- 2g, continuous update(0x00)
    config[0] = 0x23;
    config[1] = 0x00;
    write(self->fd, config, 2);
    sleep(1);
}

void lsm303_start_magnetometer(Lsm303 *self)
{
    char config[2] = {0};
    // Get I2C device, LSM303DLHC MAGNETO I2C address is 0x1E(30)
    ioctl(self->fd, I2C_SLAVE, 0x1E);

    // Select MR register(0x02)
    // Continuous conversion(0x00)
    config[0] = 0x02;
    config[1] = 0x00;
    write(self->fd, config, 2);
    // Select CRA register(0x00)
    // Data output rate = 15Hz(0x10)
    config[0] = 0x00;
    config[1] = 0x10;
    write(self->fd, config, 2);
    // Select CRB register(0x01)
    // Set gain = +/- 1.3g(0x20)
    config[0] = 0x01;
    config[1] = 0x20;
    write(self->fd, config, 2);
    sleep(1);
}


bool lsm303_get_heading(Lsm303 *self, double roll, double pitch, double *heading)
{
    char reg[1] = {0x28};
    char data[1] = {0};
    // Read 6 bytes of data
    // msb first
    // Read xMag msb data from register(0x03)
    reg[0] = 0x03;
    write(self->fd, reg, 1);
    read(self->fd, data, 1);
    char data1_0 = data[0];

    // Read xMag lsb data from register(0x04)
    reg[0] = 0x04;
    write(self->fd, reg, 1);
    read(self->fd, data, 1);
    char data1_1 = data[0];

    // Read yMag msb data from register(0x05)
    reg[0] = 0x05;
    write(self->fd, reg, 1);
    read(self->fd, data, 1);
    char data1_2 = data[0];

    // Read yMag lsb data from register(0x06)
    reg[0] = 0x06;
    write(self->fd, reg, 1);
    read(self->fd, data, 1);
    char data1_3 = data[0];

    // Read zMag msb data from register(0x07)
    reg[0] = 0x07;
    write(self->fd, reg, 1);
    read(self->fd, data, 1);
    char data1_4 = data[0];

    // Read zMag lsb data from register(0x08)
    reg[0] = 0x08;
    write(self->fd, reg, 1);
    read(self->fd, data, 1);
    char data1_5 = data[0];

    // Convert the data
    int xMag = (data1_0 * 256 + data1_1);
    if(xMag > 32767)
    {
        xMag -= 65536;
    }   

    int yMag = (data1_4 * 256 + data1_5) ;
    if(yMag > 32767)
    {
        yMag -= 65536;
    }

    int zMag = (data1_2 * 256 + data1_3) ;
    if(zMag > 32767)
    {
        zMag -= 65536;
    }
#if 0
    // Output data to screen
    printf("Magnetic field in X-Axis : %d \n", xMag);
    printf("Magnetic field in Y-Axis : %d \n", yMag);
    printf("Magnetic field in Z-Axis : %d \n", zMag);
#endif
     // Calculate the angle of the vector y,x

    double xHoriz, yHoriz;
    xHoriz = xMag * cos(pitch) 
	   + yMag * sin(roll)*sin(pitch);
    yHoriz = yMag * cos(roll)
	   + zMag * sin(roll);
    *heading = (atan2(yHoriz, xHoriz) * 180.0) / M_PI;

    // Normalize to 0-360
    if (*heading < 0) {
        *heading = 360 + *heading;
    }
    return true;
}

#if 0
int main(int argc, char **argv)
{
    Lsm303 dev;
    void *rv;
    double heading;

    rv = lsm303_init(&dev, "/dev/i2c-0");
    if(!rv){
        printf("Couldn't open bus %s, bailing out\n","/dev/i2c-0");
        exit(EXIT_FAILURE);
    }

    lsm303_start_magnetometer(&dev);
    while(1){
        lsm303_get_heading(&dev, &heading);
        printf("Heading: %f\r",heading);
        fflush(stdout);
        sleep(1);
    }
    exit(EXIT_SUCCESS);
}
#endif
