/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include "data-source.h"
#include "fg-data-source.h"

static bool fg_data_source_frame(FGDataSource *self, uint32_t dt);
static FGDataSource *fg_data_source_dispose(FGDataSource *self);
static DataSourceOps fg_data_source_ops = {
    .frame = (DataSourceFrameFunc)fg_data_source_frame,
    .dispose = (DataSourceDisposeFunc)fg_data_source_dispose
};

FGDataSource *fg_data_source_new(int port)
{
    FGDataSource *self;

    self = calloc(1, sizeof(FGDataSource));
    if(self){
        if(!fg_data_source_init(self, port)){
            free(self);
            return NULL;
        }
    }
    return self;
}

FGDataSource *fg_data_source_init(FGDataSource *self, int port)
{
    if(!data_source_init(DATA_SOURCE(self), &fg_data_source_ops))
        return NULL;

    self->fglink = flightgear_connector_new(port);
    if(!self->fglink)
        return NULL;
    flightgear_connector_set_nonblocking(self->fglink);

    self->port = port;
    return self;
}

void fg_data_source_banner(FGDataSource *self)
{
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);


    printf("Waiting for first packet from FlightGear\n");
    printf("Be sure to:\n");
    printf("1. have basic_proto.xml in $FG_ROOT/Protocol\n");
    printf("2. Run FlightGear(fgfs) with --generic=socket,out,5,%sLOCAL_IP%s,%d,udp,basic_proto\n",
        "\x1B[1;31m",
        "\x1B[0m",
        self->port
    );
    printf("Be sure to replace %sLOCAL_IP%s with the IP of the local machine, one of:\n",
        "\x1B[1;31m",
        "\x1B[0m"
    );

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if(!strcmp(ifa->ifa_name, "lo")) continue;
            printf("\t%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}

static FGDataSource *fg_data_source_dispose(FGDataSource *self)
{
    if(self->fglink)
        flightgear_connector_free(self->fglink);
    return self;
}

static bool fg_data_source_frame(FGDataSource *self, uint32_t dt)
{
    FlightgearPacket packet;
    bool rv;

    rv = flightgear_connector_get_packet(self->fglink, &packet);
    if(!rv)
        return false;

    DATA_SOURCE(self)->latitude = packet.latitude;
    DATA_SOURCE(self)->longitude = packet.longitude;
    DATA_SOURCE(self)->altitude = packet.altitude;

    DATA_SOURCE(self)->airspeed = packet.airspeed;
    DATA_SOURCE(self)->vertical_speed = packet.vertical_speed;

    DATA_SOURCE(self)->roll = packet.roll;
    DATA_SOURCE(self)->pitch = packet.pitch;
    DATA_SOURCE(self)->heading = packet.heading;

    DATA_SOURCE(self)->slip_rad = packet.side_slip;

    DATA_SOURCE(self)->rpm = packet.rpm;
    DATA_SOURCE(self)->fuel_flow = packet.fuel_flow;
    DATA_SOURCE(self)->oil_temp = packet.oil_temp;
    DATA_SOURCE(self)->oil_press = packet.oil_px;
    DATA_SOURCE(self)->cht = packet.cht;
    DATA_SOURCE(self)->fuel_px = packet.fuel_px;
    DATA_SOURCE(self)->fuel_qty = packet.fuel_qty;

    return true;
}
