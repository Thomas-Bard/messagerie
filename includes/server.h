#pragma once

#include "commons.h"
#include "net_types.h"
#include "data_types.h"

typedef struct {
	int _fd;
	char _name[11];
} Client, *PClient;

void* handleClientMessages(void*);
void* handleClientConnections(void*);
void* clientThread(void*);
void handleServer(void);
void handleMessage(int, PVarArray, char*);



