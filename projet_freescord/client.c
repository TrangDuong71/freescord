/* Nom : DUONG THI THUY TRANG
 * Numero: 12306149
 * Je déclare qu'il s'agit de mon propre travail.
 * Ce travail a été réalisé intégralement par un être humain. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include "buffer/buffer.h"
#include "utils.h"

#define PORT 4321

int connect_serveur_tcp(char *addr, uint16_t port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) 
        return -1;
    struct sockaddr_in srv = {
        .sin_family = AF_INET,
        .sin_port   = htons(port)
    };
    inet_pton(AF_INET, addr, &srv.sin_addr);
    if (connect(s,(struct sockaddr*)&srv,sizeof(srv))<0) {
        close(s); 
        return -1;
    }
    return s;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <IP_serveur>\n", argv[0]);
        return 1;
    }
    int sock = connect_serveur_tcp(argv[1], PORT);
    if (sock < 0) { perror("connect"); return 1; }

    buffer *b = buff_create(sock, 512);
    char line[1024];

    //Lire message de bienvenue 
    while (buff_fgets(b, line, sizeof(line))) {
        if (strcmp(line, "\r\n")==0 || strcmp(line, "\n")==0) 
            break;
        crlf_to_lf(line);
        fputs(line, stdout);
    }

    // Boucle pseudonyme 
    char resp[5] = {0};
    while (1) {
        printf("Choisissez un pseudonyme: ");
        if (!fgets(line, sizeof(line), stdin)) 
        {
            close(sock);
            return 0;
        }
        lf_to_crlf(line);
        write(sock, line, strlen(line));

        if (read(sock, resp, 4) != 4) 
        {
            printf("Serveur déconnecté\n");
            close(sock);
            return 1;
        }
        
        // resp contient par exemple "0 \r\n"
        printf("-------> Code reçu : %c <--------\n", resp[0]);

        resp[2] = '\0';
        switch (resp[0]) {
            case '0': goto nick_ok;
            case '1': printf("========= Déjà pris ========\n");   
                        break;
            case '2': printf("======== Interdit ==========\n");      
                        break;
            case '3': printf("========== Syntaxe incorrecte========\n"); 
                        break;
            default:  printf("========== Réponse inattendue: '%c'==========\n", resp[0]);
        }
    }
    nick_ok:

    // Boucle de chat 
    struct pollfd pf[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = sock,         .events = POLLIN }
    };
    while (1) {
        if (poll(pf, 2, -1) < 0) break;

        if (pf[0].revents & POLLIN) {
            if (!fgets(line, sizeof(line), stdin)) break;
            lf_to_crlf(line);
            write(sock, line, strlen(line));
        }
        if (pf[1].revents & (POLLIN|POLLHUP)) {
            char recv[1024];
            if (buff_fgets(b, recv, sizeof(recv))) {
                crlf_to_lf(recv);
                printf("[RECV] → %s", recv);
            } else {
                printf("\n[INFO] Serveur déconnecté\n");
                break;
            }
        }
    }

    buff_free(b);
    close(sock);
    return 0;
}
