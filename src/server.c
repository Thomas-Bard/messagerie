#include "../includes/server.h"
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

void handleServer(void) {
	fflush(stdin);
	system("clear");
	printf("== CONFIGURATION DU SERVEUR ==\n");
	printf("Port à utiliser :");
	int port = 0;
	scanf("%d", &port);
	fflush(stdin);

	port = htons(port);
	struct sockaddr_in addr;
	addr.sin_port = port;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
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
	VarArray ClientsArray = { 0 };
	PVarArray ClientsArrayPointer = &ClientsArray;
	ARRAY_TCreate(PClient, &ClientsArray);
	
	char args[sizeof(PVarArray) + sizeof(int)] = { 0 };
	memcpy(args, &ClientsArrayPointer, sizeof(PVarArray));
	memcpy(args + sizeof(PVarArray), &socket_fd, sizeof(int));

	pthread_t connection_handler = 0;
	pthread_create(&connection_handler, NULL, handleClientConnections, args);

	while (true);
}

void* handleClientConnections(void* args) {
	PVarArray clientsArray = NULL;
	int serverFd = 0;
	memcpy(&clientsArray, args, sizeof(PVarArray));
	memcpy(&serverFd, args + sizeof(PVarArray), sizeof(int));

	printf("[INFO] En attente de connexions clients...\n");

	while (true) {
		int client_socket = accept(serverFd, NULL, NULL);
		PClient client = (PClient)calloc(1, sizeof(Client));
		if (client == NULL) {
			perror("[ERREUR] Erreur lors de la création du client. Abandon de la procédure et fermeture de la connexion");
			shutdown(client_socket, SHUT_RDWR);
			close(client_socket);
			continue;
		}
		client->_fd = client_socket;
		client->initialized = false;
		if (ARRAY_Append(clientsArray, &client) < 0) {
			perror("[ERREUR] Impossible d'enregistrer le client. Abandon de la procédure et fermeture de la connexion");
			shutdown(client_socket, SHUT_RDWR);
			close(client_socket);
			free(client);
			continue;
		}
		char args[sizeof(PVarArray) + sizeof(PClient)] = { 0 };

		memcpy(args, &clientsArray, sizeof(PVarArray));
		memcpy(args + sizeof(PVarArray), &client, sizeof(PClient));

		pthread_t clientThreadId = 0;
		pthread_create(&clientThreadId, NULL, clientThread, args);

		printf("[INFO] Nouvelle connexion établie avec succès. En attente d'authentification ultérieure.\n");
	}
}

void* clientThread(void* args) {
	PVarArray clientsArray = NULL;
	PClient client = NULL;
	memcpy(&clientsArray, args, sizeof(PVarArray));
	memcpy(&client, args + sizeof(PVarArray), sizeof(PClient));

	while (true) {
		char buffer[PACKET_HEADER_SIZE] = { 0 };
		ssize_t bytes_read = recv(client->_fd, buffer, PACKET_HEADER_SIZE, 0);
		if (bytes_read <= 0) {
			handleDisconnection(clientsArray, client);
			return NULL;
		}

		uint8_t packet_type = 0;
		uint64_t payload_size = 0;
		memcpy(&packet_type, buffer, sizeof(uint8_t));
		memcpy(&payload_size, buffer + 1, sizeof(uint64_t));

		switch (packet_type) {
			case PACKET_TYPE_AUTH:
				handleAuthentication(clientsArray, client, payload_size);
				break;
			case PACKET_TYPE_DEAUTH:
				handleDisconnection(clientsArray, client);
				break;
			case PACKET_TYPE_MESSAGE:
				if (!client->initialized) break;
				printf("[DEBUG] Nouveau message de [%s]\n", client->_name);
				handleMessage(clientsArray, client, payload_size);
				break;
		}
		// TODO Réception des messages, première partie avec traitement du header 
		// TODO Réception des messages, seconde partie avec traitement en fonction du header 
		// Traitement par lecture des octets suivants sur le fd de 1 à n (n taille précisée dans le header oct 1 à 8)
		// RAPPEL Structure header : TAILLE : 9 octets : OCTET 0 = TYPE PAQUET ; OCTET 1 à 8 = taille charge utile 
	}
}

void handleAuthentication(PVarArray clients, PClient client, uint64_t payload_size) {
	if (client->initialized) return;
	if (payload_size > USERNAME_MAX_LENGTH + 1) {
		sendError(PACKET_TYPE_ERROR_INVALID_USERNAME, client);
		return;
	}
	char* payload = (char*)calloc(payload_size, sizeof(char));
	if (payload == NULL) {
		sendError(PACKET_TYPE_ERROR_INTERNAL_SERVER, client);
		return;
	}
	read(client->_fd, payload, payload_size);
	bool is_name_correct = true;
	uint8_t auth_buffer[PACKET_HEADER_SIZE + 31]= { 0 };
	auth_buffer[0] = PACKET_TYPE_CLIENT_AUTH;
	memcpy(auth_buffer + PACKET_HEADER_SIZE, payload, payload_size);
	for (uint64_t i = 1; i < clients->size; i++) {
		PClient current_client = ARRAY_TIndex(PClient, clients, i);
		if (current_client == NULL) break;
		if (current_client->_fd == client->_fd) continue;
		if (!current_client->initialized) break;
		if (strcmp(payload, current_client->_name) == 0) {
			is_name_correct = false;
			break;
		}

	}
	if (!is_name_correct) {
		sendError(PACKET_TYPE_ERROR_INVALID_USERNAME, client);
		free(payload);
		return;
	}
	for (uint8_t i = 1; i < clients->size; i++) {
		PClient current_client = ARRAY_TIndex(PClient, clients, i);
		if (current_client == NULL) break;
		if (current_client->_fd == client->_fd) continue;
		if (!current_client->initialized) continue;
		send(current_client->_fd, auth_buffer, PACKET_HEADER_SIZE + 31, 0);
	}
	memcpy(client->_name, payload, payload_size);
	client->initialized = true;
	free(payload);
	printf("[INFO] Nouveau client authentifié : %s\n", client->_name);
	sendError(PACKET_TYPE_AUTH_SUCCESS, client);
	return; 
}

void handleDisconnection(PVarArray clientsArray, PClient client_disconnected) {
	client_disconnected->initialized = false;
	printf("[INFO] Déconnexion de [%s]\n", client_disconnected->_name);
	shutdown(client_disconnected->_fd, SHUT_RDWR);
	close(client_disconnected->_fd);
	for (uint64_t i = 1; i < clientsArray->size; i++) {
		PClient client = ARRAY_TIndex(PClient, clientsArray, i);
		if (client == NULL) break;
		if (client->_fd == client_disconnected->_fd) { 
			ARRAY_Remove(clientsArray, i);
			printf("[INFO] Déconnexion effectuée avec succès !\n");
			sendDisconnectionMessage(clientsArray, client_disconnected);
			free(client_disconnected);
			
		}
	}
}

void sendError(uint8_t error_code, PClient client) {
	char buffer[PACKET_HEADER_SIZE] = { 0 };
	memcpy(buffer, &error_code, sizeof(uint8_t));
	send(client->_fd, buffer, sizeof(buffer), 0);
}

void sendDisconnectionMessage(PVarArray clientsArray, PClient client_disconnected) {
	uint64_t name_length = strlen(client_disconnected->_name) + 1;
	uint8_t *buffer = (uint8_t*)calloc(PACKET_HEADER_SIZE + name_length, sizeof(uint8_t));
	if (buffer == NULL) {
		perror("[ERREUR] Erreur lors de l'émission du paquet de déconnexion\n");
		return;
	}
	buffer[0] = PACKET_TYPE_CLIENT_DEAUTH;
	memcpy(buffer + PACKET_HEADER_PL_SIZE_INDEX, &name_length, sizeof(uint64_t));
	memcpy(buffer + PACKET_HEADER_SIZE, client_disconnected->_name, name_length - 1);
	for (uint64_t i = 1; i < clientsArray->size; i++) {
		PClient client = ARRAY_TIndex(PClient, clientsArray, i);
		if (client == NULL) break;
		if (!client->initialized) continue;
		send(client->_fd, buffer, PACKET_HEADER_SIZE + name_length, 0);
	}
	free(buffer);
	return;
}

void handleMessage(PVarArray clientsArray, PClient client, uint64_t payload_size) {
	if (payload_size > MESSAGE_MAX_LENGTH + 1) {
		sendError(PACKET_TYPE_ERROR_MSG_TOO_LONG, client);
		return;
	}
	uint8_t* buffer = (uint8_t*)calloc(PACKET_HEADER_SIZE + payload_size + 32, sizeof(char));
	if (buffer == NULL) {
		sendError(PACKET_TYPE_ERROR_INTERNAL_SERVER, client);
		return;
	}
	read(client->_fd, buffer + PACKET_HEADER_SIZE + sizeof(client->_name), payload_size);
	buffer[0] = PACKET_TYPE_MESSAGE;
	payload_size += 32;
	memcpy(buffer + PACKET_HEADER_PL_SIZE_INDEX, &payload_size, sizeof(uint64_t));
	memcpy(buffer + PACKET_HEADER_SIZE, client->_name, sizeof(client->_name));
	printf("[DEBUG] Message de [%s] : [%s]\n", client->_name, buffer + PACKET_HEADER_SIZE + sizeof(client->_name));
	for (uint64_t i = 1; i < clientsArray->size; i++) {
		PClient actual_client = ARRAY_TIndex(PClient, clientsArray, i);
		if (actual_client == NULL) break;
		if (!actual_client->initialized) continue;
		if (actual_client->_fd == client->_fd) continue;
		send(actual_client->_fd, buffer, PACKET_HEADER_SIZE + payload_size + 32, 0);
	}
	return;
}

/*
void* handleClientConnections(void* args) {
    int socket_fd = _DFINT args;
    int port = _DFINT (args + sizeof(int));
    printf("fd : %d port : %d\n", socket_fd, port);
    PVarArray array = NULL;
    memcpy(&array, &((char*)args)[2*sizeof(int)], sizeof(PVarArray));
    printf("After MEMCPY\n");
    struct sockaddr_in addr;
    addr.sin_port = port;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;

    socklen_t addr_size = sizeof(addr);
    
    while (1) {
        int fd = accept(socket_fd, (struct sockaddr*)&addr, &addr_size);
	if (fd < 0) continue;
        printf("[INFO] Nouvelle connexion acceptée.\n");
        ARRAY_Append(array, &fd);
	char args[sizeof(int) + sizeof(PVarArray)] = { 0 };
	memcpy(args, &fd, sizeof(int));
	memcpy(args + sizeof(int), &array, sizeof(array));
	pthread_t temp_id;
	pthread_create(&temp_id, NULL, clientThread, args);
	printf("Thread créé, taille de la VarArray : %llu ; Pointeur : %p\n", array->size, array);
    }
}
*/
// Structure des arguments du thread :
// ADRESSE | VALEUR
// 0x00    | fd client
// sof(int)| Array fd clients
/*
void* clientThread(void* args) {
	int self_fd = _DFINT args;
	printf("Nouveau thread : %d\n", self_fd);
	PVarArray clients_fd = NULL;
	memcpy(&clients_fd, args + sizeof(int), sizeof(clients_fd));
	printf("Taille vararray : %lu ; Pointeur : %p\n", clients_fd->size, clients_fd);	
	while (1) {
		//printf("BOUCLE FONCTIONNELLE %d", self_fd);
		char buffer[MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE] = { 0 };
		ssize_t bytes_read = recv(self_fd, buffer, MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE, 0);
		if (bytes_read < PACKET_HEADER_SIZE + 1) {
			printf("Message reçu mais taille inférieure : %ld octets\n", bytes_read);
			if (bytes_read == 0) {
				handleDisconnection(clients_fd, self_fd);
				printf("[INFO] Déconnexion du client effectuée avec succès !\n");
				return NULL;
			}
			continue;
		}
		printf("[INFO] Nouveau paquet depuis %d\n", self_fd);
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
*/
/*
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
*/


/*
void handleDisconnection(PVarArray clients_socket, int disconnected_socket) {
	printf("[INFO] Client déconnecté. Fermeture socket... %d\n", disconnected_socket);
	shutdown(disconnected_socket, SHUT_RDWR);
	close(disconnected_socket);
	for (uint64_t i = 0; i < clients_socket->size; i++) {
		void* result = ARRAY_Index(clients_socket, i);
		if (result == NULL) {
			printf("[ERREUR] Erreur lors de la suppression du socket dans la base de donnée\n");
			return;
		}
		if (disconnected_socket == _DFINT result) {
			ARRAY_Remove(clients_socket, i);
			printf("[INFO] Socket supprimé avec succès\n");
			return;
		}
	}

}
*/
