#include "../includes/client.h"


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
    struct sockaddr_in addr;
    inet_pton(AF_INET, ip, &addr);
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
    printf("Connexon établie ! Entrez un pseudo pour discuter (10 lettres) :");
    char pseudo[11] = { 0 };
    fflush(stdin);
    scanf("%s", pseudo);
    for (int i = 0; i < 11; i++) {
        if (pseudo[i] == 0xa) {
            pseudo[i] = 0;
            break;
        }
    }
    system("clear");
    printf("== Bienvenue ! ==\n");
    pthread_t message_handler;
    pthread_create(&message_handler, NULL, handleMessages, (void*)&socket_fd);
    char buffer[MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE] = { 0 };
    memcpy(buffer, pseudo, 10);
    memset(buffer + 11, PACKET_TYPE_MESSAGE, 1);
    fflush(stdin);
    sleep(1);
    while (1) {
        fgets(buffer + PACKET_HEADER_SIZE, MESSAGE_MAX_LENGTH - 1, stdin);
        ssize_t bytes_send = send(socket_fd, buffer, MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE, 0);
        printf("VOUS>%s", buffer+PACKET_HEADER_SIZE);
        memset(buffer + PACKET_HEADER_SIZE, 0, MESSAGE_MAX_LENGTH);
        fflush(stdin);
    }
}

void* handleMessages(void* args) {
    int fd = _DFINT args;
    while (1) {
        char buffer[MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE] = { 0 };
        while (recv(fd, buffer, MESSAGE_MAX_LENGTH + PACKET_HEADER_SIZE, 0) < PACKET_HEADER_SIZE + 1) { ;; }
        uint8_t req_type = 0;
        memcpy(&req_type, buffer + PACKET_HEADER_TYPE_INDEX, 1);
        if (req_type == PACKET_TYPE_MESSAGE) {
            printf("\n%s>%s\n", buffer, buffer + PACKET_HEADER_SIZE);
        }
    }
}
