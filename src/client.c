#include "../includes/client.h"
#include <sys/socket.h>


void handleClient(void) {
    system("clear");
    fflush(stdin);
    printf("== CONNEXION AU SERVEUR ==\n");
    printf("Adresse IP :");
    char ip[20] = { 0 };
    scanf("%s", ip);
    fflush(stdin);
    printf("Port :");
    int port = 0;
    scanf("%d", &port);
    for (int i = 0; i < 20; i++) {
        if (ip[i] == 0xa) {
            ip[i] = 0;
            break;
        }
    }
    port = htons(port);
    struct sockaddr_in addr;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = port;
    addr.sin_family = AF_INET;

    int socket_fd = 0;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Impossible d'ouvrir le socket. Abandon...\n");
        return;
    }

    if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
        printf("Connexion impossible ! Abandon...\n");
        return;
    }
    printf("Connexon établie ! Entrez un pseudo pour discuter (30 lettres) :");
    char pseudo[31] = { 0 };
    __fpurge(stdin);
    scanf("%s", pseudo);
    for (int i = 0; i < 11; i++) {
        if (pseudo[i] == 0xa) {
            pseudo[i] = 0;
            break;
        }
	if (pseudo[i] == ' ') {
		pseudo[i] = '-';
	}
    }
    printf("Authentification en cours...\n");
    uint8_t auth_buffer[PACKET_HEADER_SIZE + 31] = { 0 };
    auth_buffer[0] = PACKET_TYPE_AUTH;
    uint64_t size = sizeof(pseudo);
    memcpy(auth_buffer + 1, &size, sizeof(uint64_t));
    memcpy(auth_buffer + PACKET_HEADER_SIZE, pseudo, sizeof(pseudo));
    send(socket_fd, auth_buffer, PACKET_HEADER_SIZE + size, 0);
    uint8_t response_buffer[PACKET_HEADER_SIZE] = { 0 };
    while (recv(socket_fd, response_buffer, PACKET_HEADER_SIZE, 0) < PACKET_HEADER_SIZE);

    if (response_buffer[0] != PACKET_TYPE_AUTH_SUCCESS) {
	printf("Une erreur s'est produite, déconnexion...\n");
	handleError(response_buffer[0], &socket_fd);
	return;
    }

    system("clear");
    printf("== Bienvenue ! ==\n");
    int* socket_fd_ptr = &socket_fd;
    pthread_t message_handler;
    pthread_create(&message_handler, NULL, handleMessages, (void*)socket_fd_ptr);
    uint8_t buffer[MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE] = { 0 };
    buffer[0] = PACKET_TYPE_MESSAGE;
    __fpurge(stdin);
    sleep(1);
    while (1) {
        if (fgets(buffer + PACKET_HEADER_SIZE, MESSAGE_MAX_LENGTH - 1, stdin) == NULL) continue;
	if (socket_fd == 0) return;
	uint64_t msg_len = strlen(buffer + PACKET_HEADER_SIZE);
	for (uint64_t i = 0; i < msg_len; i++) {
		if (buffer[i + PACKET_HEADER_SIZE] == '\n') buffer[i + PACKET_HEADER_SIZE] = 0;
	}
	memcpy(buffer + PACKET_HEADER_PL_SIZE_INDEX, &msg_len, sizeof(uint64_t));
        ssize_t bytes_send = send(socket_fd, buffer, MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE, 0);
        memset(buffer + PACKET_HEADER_SIZE, 0, MESSAGE_MAX_LENGTH);
        fflush(stdin);
    }
}

void* handleMessages(void* args) {
    int* fd = (int*)args;
    while (1) {
        char buffer[MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE] = { 0 };
        ssize_t bytes_recv = recv(*fd, buffer, MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE, 0);

	if (bytes_recv == 0) {
		handleError(PACKET_TYPE_ERROR_INTERNAL_SERVER, fd);
		return NULL;
	}

	if (bytes_recv < PACKET_HEADER_SIZE) continue;

        uint8_t req_type = 0;
        memcpy(&req_type, buffer + PACKET_HEADER_TYPE_INDEX, 1);
        if (req_type == PACKET_TYPE_MESSAGE) {
            printf("\n%s>%s\n", buffer + PACKET_HEADER_SIZE, buffer + PACKET_HEADER_SIZE + 31);
        } else if ((req_type & 0xC0) == 0xC0) 				// Un code d'erreur est sous la forme 0xCX de ce fait si une erreur est envoyée, l'opération 0xCX & 0xC0 équivaudra toujours 0xC0
	{
		handleError(req_type, fd);
		if (*fd == 0) return NULL;
		continue;
	} else if (req_type == PACKET_TYPE_CLIENT_DEAUTH) {
		printf("[%s] A quitté le salon\n", buffer + PACKET_HEADER_SIZE);
		continue;
	} else if (req_type == PACKET_TYPE_CLIENT_AUTH) {
		printf("[%s] A rejoint le salon\n", buffer + PACKET_HEADER_SIZE);
		continue;
	}
    }
}

void handleError(uint8_t error, int* socket_fd) {
	switch (error) {
		case PACKET_TYPE_ERROR_INVALID_USERNAME:
			printf("Nom d'utilisateur rejeté par le serveur.\n");
			shutdown(*socket_fd, SHUT_RDWR);
			close(*socket_fd);
			*socket_fd = 0;
			return;
		case PACKET_TYPE_ERROR_INTERNAL_SERVER:
			printf("Erreur serveur interne.\n");
			shutdown(*socket_fd, SHUT_RDWR);
			close(*socket_fd);
			*socket_fd = 0;
			return;
		case PACKET_TYPE_ERROR_MSG_TOO_LONG:
			printf("Mollo l'asticot ! Ca fait beaucoup de caractères !\n");
			return;
	}
}

