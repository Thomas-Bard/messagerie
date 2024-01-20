#pragma once
#include "commons.h"

#define MESSAGE_MAX_LENGTH 16384

#define PACKET_HEADER_SIZE 12
#define PACKET_HEADER_TYPE_INDEX 11

#define PACKET_TYPE_CONNECTION 0xAA
#define PACKET_TYPE_DISCONNECTION 0xAB
#define PACKET_TYPE_REMOTE_DISCONNECTED 0xAC
#define PACKET_TYPE_REMOTE_LOST_CONNECTION 0xAD
#define PACKET_TYPE_REMOTE_CONNECTION 0xAE
#define PACKET_TYPE_SERVER_CLOSED 0xAF

#define PACKET_TYPE_MESSAGE 0xBA

#define PACKET_TYPE_ERROR_MSG_TOO_LONG 0xCA

typedef struct {
	char header[12];
	char data[MESSAGE_MAX_LENGTH];
	size_t size;
} Packet, *PPacket;
