#pragma once

#include "commons.h"
#include "net_types.h"
#include "data_types.h"

typedef struct {
	int _fd;
	char _name[31];
	bool initialized;
} Client, *PClient;


void handleDisconnection(PVarArray clients, PClient client_disconnected);

void* handleClientConnections(void*);
void* clientThread(void*);
void handleServer(void);
void handleMessage(PVarArray clientsArray, PClient client, uint64_t payload_size);
void handleAuthentication(PVarArray clients, PClient client, uint64_t payload_size);

void sendError(uint8_t error_code, PClient client);
void sendDisconnectionMessage(PVarArray clientsArray, PClient client_disconnected);

bool checkUsername(PVarArray clients_array, char* username);


