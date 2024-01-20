#include "../includes/server.h"
#include "../includes/client.h"

int main(int argc, char** argv) {
    int choix = 0;
    printf("== MESSAGERIE ==\n");
    printf("1. Se Connecter à un serveur\n2. Héberger un serveur\nVotre choix :");
    scanf("%d", &choix);
    switch (choix)
    {
    case 1:
        handleClient();
        break;
    case 2:
        handleServer();
        break;
    default:
        printf("Choix invalide !\n");
        break;
    }
}
