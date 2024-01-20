#include "../includes/server.h"
#include <fcntl.h>
#include <sys/socket.h>

void handleServer(void) {
    fflush(stdin);
    system("clear");
    printf("== CONFIGURATION DU SERVEUR ==\n");
    printf("Port à utiliser :");
    int port = 0;
    scanf("%d", &port);
    fflush(stdin);
    
    struct sockaddr_in addr;
    addr.sin_port = port;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    socklen_t addr_size = sizeof(addr);

    int socket_fd = 0;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Erreur lors de l'ouverture du socket. Abandon...\n");
        return;
    }

    if (bind(socket_fd, (struct sockaddr*)&addr, addr_size) < 0) {
        printf("Erreur lors de l'association du socket. Abandon...\n");
        return;
    }

    if (listen(socket_fd, 5) < 0) {
        printf("Erreur lors de l'écoute sur le port sélectionné. Abandon...\n");
        return;
    }

    printf("Serveur en route !\n");
    VarArray clients_fd = { 0 };
    ARRAY_Create(&clients_fd, sizeof(int));
    PVarArray cfd_ptr = &clients_fd;
    //pthread_t request_handler;
    //pthread_create(&request_handler, NULL, handleClientMessages, &clients_fd);
    char args[2*sizeof(int) + sizeof(PVarArray)] = { 0 };
    memcpy(args, &socket_fd, sizeof(int));
    memcpy(args + sizeof(int), &port, sizeof(int));
    memcpy(args + 2*sizeof(int), &cfd_ptr, sizeof(PVarArray));

    pthread_t connection_handler;
    pthread_create(&connection_handler, NULL, handleClientConnections, args);
    while (1) {

    }
}

void* handleClientConnections(void* args) {
    int socket_fd = _DFINT args;
    int port = _DFINT (args + sizeof(int));
    printf("fd : %d port : %d\n", socket_fd, port);
    PVarArray array = NULL;
    memcpy(&array, &((char*)args)[2*sizeof(int)], sizeof(PVarArray));
    printf("After MEMCPY\n");
    struct sockaddr_in addr;
    addr.sin_port = port;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    socklen_t addr_size = sizeof(addr);
    LPVarArray lp_vararr = &array;
    
    while (1) {
        int fd = accept(socket_fd, (struct sockaddr*)&addr, &addr_size);
	if (fd < 0) continue;
        printf("[INFO] Nouvelle connexion acceptée.\n");
        ARRAY_Append(array, &fd);
        printf("Append effectué !\n");
	char args[sizeof(int) + sizeof(PVarArray)] = { 0 };
	memcpy(args, &fd, sizeof(int));
	memcpy(args + sizeof(int), &array, sizeof(array));
	pthread_t temp_id;
	pthread_create(&temp_id, NULL, clientThread, args);
	printf("Thread créé, taille de la VarArray : %llu ; Pointeur : %p\n", array->size, array);
    }
}

// Structure des arguments du thread :
// ADRESSE | VALEUR
// 0x00    | fd client
// sof(int)| Array fd clients

void* clientThread(void* args) {
	int self_fd = _DFINT args;
	printf("Nouveau thread : %d\n", self_fd);
	PVarArray clients_fd = NULL;
	memcpy(&clients_fd, args + sizeof(int), sizeof(clients_fd));
	printf("Taille vararray : %lu ; Pointeur : %p\n", clients_fd->size, clients_fd);	
	while (1) {
		//printf("BOUCLE FONCTIONNELLE %d", self_fd);
		char buffer[MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE] = { 0 };
		ssize_t bytes_read = recv(self_fd, buffer, MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE, MSG_WAITALL);
		if (bytes_read < PACKET_HEADER_SIZE + 1) {
			printf("Message reçu mais taille inférieure : %ld octets\n", bytes_read);
			if (bytes_read == 0) {
				printf("Connexion fermée par le client !\n");
				close(self_fd);
				for (size_t i = 0; i < clients_fd->size; i++) {
					void* result = ARRAY_Index(clients_fd, i);
					if (result == NULL) {
						printf("Problème de taille, result vaut NULL\n");
						return NULL;
					}
					printf("Comparaison %d == %d\n", self_fd, _DFINT result);
					if (_DFINT result == self_fd) {
						printf("Suppression du socket !\n");
						ARRAY_Remove(clients_fd, i);
						return NULL;
					}
				}
			}
			continue;
		}
		printf("Nouveau paquet depuis %d\n", self_fd);
		uint8_t type = 0;
		memcpy(&type, buffer + PACKET_HEADER_TYPE_INDEX, 1);
		switch (type) {
			case PACKET_TYPE_MESSAGE:
				handleMessage(self_fd, clients_fd, buffer);
				break;
			default:
				break;
		}
	}
	return NULL;
}

void handleMessage(int self_fd, PVarArray clients_fd, char* buffer) {
	char pseudo[11] = { 0 };
	memcpy(pseudo, buffer, 11);
	printf("[LOG] %s : %s", pseudo, buffer + PACKET_HEADER_SIZE);
	for (size_t i = 0; i < clients_fd->size; i++) {
		void* result = ARRAY_Index(clients_fd, i);
		if (result == NULL) break;
		int actual_fd = _DFINT result;
		if (actual_fd == self_fd) continue;
		send(actual_fd, buffer, PACKET_HEADER_SIZE + MESSAGE_MAX_LENGTH, 0);
	}
}

// TODO Somehow create a message reading system that works

