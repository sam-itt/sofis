#ifndef FLIGHTGEAR_CONNECTOR_H
#define FLIGHTGEAR_CONNECTOR_H

#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


typedef struct __attribute__((__packed__))  {
    float latitude;
    float longitude;
    int altitude;
    int airspeed;
    float vertical_speed;
    float roll;
    float pitch;
    float heading;
}FlightgearPacket;

typedef struct{
	int fd;

	struct sockaddr_in myaddr;

}FlightgearConnector;



FlightgearConnector *flightgear_connector_new(int port);
FlightgearConnector *flightgear_connector_init(FlightgearConnector *self, int port);
void flightgear_connector_dispose(FlightgearConnector *self);
void flightgear_connector_free(FlightgearConnector *self);


void flightgear_connector_set_nonblocking(FlightgearConnector *self);

bool flightgear_connector_get_packet(FlightgearConnector *self, FlightgearPacket *packet);
#endif /* FLIGHTGEAR_CONNECTOR_H */
