#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "flightgear-connector.h"

FlightgearConnector *flightgear_connector_new(int port)
{
    FlightgearConnector *self;

    self = calloc(1, sizeof(FlightgearConnector));
    if(self){
        if(!flightgear_connector_init(self, port)){
            free(self);
            return NULL;
        }
    }
    return self;
}

FlightgearConnector *flightgear_connector_init(FlightgearConnector *self, int port)
{
	int optval = 1;
    int rv;

    self->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(self->fd < 0){
        return NULL;
    }


	setsockopt(self->fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

	memset(&self->myaddr, 0, sizeof(struct sockaddr_in));

	// Filling server information
	self->myaddr.sin_family = AF_INET;
	self->myaddr.sin_addr.s_addr = INADDR_ANY;
	self->myaddr.sin_port = htons(port);

    rv = bind(self->fd, (struct sockaddr*)&self->myaddr, sizeof(struct sockaddr_in));
    if(rv < 0)
        return NULL;

    return self;
}

void flightgear_connector_set_nonblocking(FlightgearConnector *self)
{
    int flags = fcntl(self->fd, F_GETFL);
    fcntl(self->fd, F_SETFL, flags | SOCK_NONBLOCK);
}

void flightgear_connector_dispose(FlightgearConnector *self)
{
   close(self->fd);
}

void flightgear_connector_free(FlightgearConnector *self)
{
    flightgear_connector_dispose(self);
    free(self);
}


static float float_swap(float value){
   int temp = htonl(*(unsigned int*)&value);
   return *(float*)&temp;
};


bool flightgear_connector_get_packet(FlightgearConnector *self, FlightgearPacket *packet)
{
    ssize_t n;
    FlightgearPacket buffer;

    memset(&buffer, 0, sizeof(FlightgearPacket));
    n = recvfrom(self->fd, &buffer, sizeof(FlightgearPacket), 0, NULL, NULL);
    if(n > 0){
        // populate structture
        packet->airspeed = ntohl(buffer.airspeed);
        packet->altitude = ntohl(buffer.altitude);
        packet->heading = float_swap(buffer.heading);
        packet->vertical_speed = float_swap(buffer.vertical_speed);
        packet->vertical_speed *= 60.0; /*Flight gear gives a value un ft/s*/
        packet->latitude = float_swap(buffer.latitude);
        packet->longitude = float_swap(buffer.longitude);
        packet->pitch = float_swap(buffer.pitch);
        packet->roll = float_swap(buffer.roll);
#if 0
        // print out data
        printf("Airspeed %i, Altitude %i\n", packet->airspeed, packet->altitude);
        printf("Position %3.2f, %3.2f\n", packet->latitude, packet->longitude);
        printf("Pitch/Roll/Heading %3.2f, %3.2f, %3.2f\n", packet->pitch, packet->roll, packet->heading);
#endif
    }

    return n == sizeof(FlightgearPacket);
}
