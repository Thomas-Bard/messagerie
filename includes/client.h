#pragma once

#include "commons.h"
#include "data_types.h"
#include "net_types.h"

void* handleMessages(void*);
void handleClient(void);
void handleError(uint8_t error, int *socket_fd);
