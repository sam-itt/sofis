/*Code from the manufacturer with marginal changes*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

#include <pthread.h>

#include "jy61.h"


static int ret;
static int fd;

static void jy61_worker(JY61 *self);
#define BAUD 115200 //115200 for JY61 ,9600 for others

int uart_open(int fd,const char *pathname)
{
    fd = open(pathname, O_RDWR|O_NOCTTY);
    if (-1 == fd)
    {
        perror("Can't Open Serial Port");
		return(-1);
	}
    else
		printf("open %s success!\n",pathname);
    if(isatty(STDIN_FILENO)==0)
		printf("standard input is not a terminal device\n");
    else
		printf("isatty success!\n");
    return fd;
}

int uart_set(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if  ( tcgetattr( fd,&oldtio)  !=  0) {
        perror("SetupSerial 1");
        printf("tcgetattr( fd,&oldtio) -> %d\n",tcgetattr( fd,&oldtio));
        return -1;
    }
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag  |=  CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch( nBits ){
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
    }

    switch( nEvent ){
        case 'o':
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'e':
        case 'E':
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'n':
        case 'N':
            newtio.c_cflag &= ~PARENB;
            break;
        default:
            break;
    }

    switch( nSpeed ){
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        case 460800:
            cfsetispeed(&newtio, B460800);
            cfsetospeed(&newtio, B460800);
            break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }

    if( nStop == 1 )
        newtio.c_cflag &=  ~CSTOPB;
    else if ( nStop == 2 )
        newtio.c_cflag |=  CSTOPB;

    newtio.c_cc[VTIME]  = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);

    if((tcsetattr(fd,TCSANOW,&newtio))!=0){
        perror("com set error");
        return -1;
    }
    printf("set done!\n");
    return 0;
}

int uart_close(int fd)
{
    assert(fd);
    close(fd);

    return 0;
}

int send_data(int  fd, char *send_buffer,int length)
{
	length=write(fd,send_buffer,length*sizeof(unsigned char));
	return length;
}

int recv_data(int fd, char* recv_buffer,int length)
{
	length=read(fd,recv_buffer,length);
	return length;
}

/*Nearly vanilla code straight form the manufacturer*/
static float a[3],w[3],Angle[3],h[3];
static void ParseData(JY61 *self, char chr)
{
    static char chrBuf[100];
    static unsigned char chrCnt=0;

    signed short sData[4];
    unsigned char i;
    char cTemp=0;
    time_t now;

    chrBuf[chrCnt++]=chr;
    if (chrCnt<11) return;

    for (i=0;i<10;i++) cTemp+=chrBuf[i];

    if((chrBuf[0]!=0x55)||((chrBuf[1]&0x50)!=0x50)||(cTemp!=chrBuf[10])){
      //  printf("Error:%x %x\r\n",chrBuf[0],chrBuf[1]);
        memcpy(&chrBuf[0],&chrBuf[1],10);
        chrCnt--;
        return;
    }

    memcpy(&sData[0],&chrBuf[2],8);
    switch(chrBuf[1]){
        case 0x51:
            for (i=0;i<3;i++)
                a[i] = (float)sData[i]/32768.0*16.0;
            time(&now);
//            printf("\r\nT:%s a:%6.3f %6.3f %6.3f ",asctime(localtime(&now)),a[0],a[1],a[2]);
            break;
        case 0x52:
            for (i=0;i<3;i++)
                w[i] = (float)sData[i]/32768.0*2000.0;
//            printf("w:%7.3f %7.3f %7.3f ",w[0],w[1],w[2]);
            break;
        case 0x53:
           pthread_mutex_lock(&self->angle_mutex);
            for (i=0;i<3;i++)
                Angle[i] = (float)sData[i]/32768.0*180.0;
//            printf("A:%7.3f %7.3f %7.3f\r",Angle[0],Angle[1],Angle[2]);
//            fflush(stdout);
            pthread_mutex_unlock(&self->angle_mutex);
//            printf("Got angle data\n");
            break;
        case 0x54:
            for (i=0;i<3;i++)
                h[i] = (float)sData[i];
//            printf("h:%4.0f %4.0f %4.0f ",h[0],h[1],h[2]);
            break;
    }
    chrCnt=0;
    return;
}

JY61 *jy61_init(JY61 *self, const char *device)
{
    bzero(self->r_buf, 1024);
    self->fd = uart_open(fd, device);
    if(fd == -1){
        fprintf(stderr,"uart_open error\n");
        return NULL;
    }

    if(uart_set(fd,BAUD,8,'N',1) == -1){
        fprintf(stderr,"uart set failed!\n");
        return NULL;
    }

    pthread_mutex_init(&self->angle_mutex, NULL);

    return self;
}

JY61 *jy61_dispose(JY61 *self)
{
    pthread_cancel(self->reader_tid);
    uart_close(self->fd);
    return self;
}

int jy61_start(JY61 *self)
{
    return pthread_create(
        &self->reader_tid,
        NULL,
        (void*)jy61_worker,
        (void*)self
    );
}

void jy61_get_attitude(JY61 *self, double *roll, double *pitch, double *yaw)
{
    pthread_mutex_lock(&self->angle_mutex);
    *roll = Angle[1];
    *pitch = Angle[0];
    *yaw = Angle[2];
    pthread_mutex_unlock(&self->angle_mutex);
}

static void jy61_worker(JY61 *self)
{
    while(1) /*TODO: Add a stop condition?*/
    {
        ret = recv_data(self->fd,self->r_buf,44);
        if(ret == -1){
            fprintf(stderr,"uart read failed!\n");
            break;
        }
		for (int i=0;i<ret;i++){
            ParseData(self, self->r_buf[i]);
        }
        usleep(1000); /*TODO: Replace with select()?*/
    }
}

#ifdef JY61_TEST
int main(void)
{
    JY61 dev;

    if(!jy61_init(&dev)){
        printf("Couldn't initialize device, bailing out\n");
        exit(EXIT_FAILURE);
    }

    jy61_start(&dev);

#if 1
    double roll,pitch,yaw;
    while(1)
    {
        jy61_get_attitude(&dev, &roll, &pitch, &yaw);
        printf("Roll: %7.3f  Pitch: %7.3f Yaw:%7.3f\r",roll,pitch, yaw);
        fflush(stdout);
        usleep(1000);
    }
#endif
    jy61_dispose(&dev);
    exit(EXIT_SUCCESS);
}
#endif
