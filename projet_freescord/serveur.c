/* Nom : DUONG THI THUY TRANG
 * Numero: 12306149
 * Je déclare qu'il s'agit de mon propre travail.
 * Ce travail a été réalisé intégralement par un être humain. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "list/list.h"
#include "buffer/buffer.h"
#include "user.h"

#define PORT 4321

static int pipefd[2];
static struct list *client_list;
static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

//  Création de la socket d'écoute 
int create_listening_sock(uint16_t port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) 
    { 
        perror("socket"); 
        return -1; 
    }
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        perror("bind"); 
        close(s); 
        return -1;
    }
    if (listen(s, SOMAXCONN) < 0) {
        perror("listen"); 
        close(s); 
        return -1;
    }
    return s;
}


void *repeater(void *arg) {
    (void)arg;
    char msg[1024];
    char sendbuf[1024 + 1];
    while (1) {
        ssize_t n = read(pipefd[0], msg, sizeof(msg) - 1);
        if (n <= 0) 
            break;
        msg[n] = '\0';

        size_t len = strlen(msg);
        if (len >= 2 && msg[len - 2] == '\r' && msg[len - 1] == '\n') {
            // déjà en CRLF
            strcpy(sendbuf, msg);
        } else if (len >= 1 && msg[len - 1] == '\n') {
            // seul LF → on glisse un CR juste avant    
            memcpy(sendbuf, msg, len - 1);
            sendbuf[len - 1] = '\r';
            sendbuf[len] = '\n';
            sendbuf[len + 1] = '\0';
        } else {
            // pas de fin de ligne → on laisse tel quel
            strcpy(sendbuf, msg);
        }

        pthread_mutex_lock(&list_lock);
        struct node *cur = client_list->first;
        while (cur) {
            int client_fd = *(int *)cur->elt;
            send(client_fd, sendbuf, strlen(sendbuf), 0);
            cur = cur->next;
        }
        pthread_mutex_unlock(&list_lock);
    }
    return NULL;
}

//authentifie puis broute le chat 
void *handle_client(void *arg) {
    struct user *u = arg;
    int fd = u->sock;

    // Message de bienvenue 
    dprintf(fd, "Bienvenue sur Freescord !\r\n");
    dprintf(fd, "Tapez \"nickname VOTRE_PSEUDO\" pour vous identifier.\r\n");
    dprintf(fd, "Une fois connecté, vous pouvez chatter librement.\r\n");
    dprintf(fd, "\r\n");

    buffer *b = buff_create(fd, 1024);
    char line[1024];

    // Validation du pseudonyme
    while (1) {
        if (!buff_fgets_crlf(b, line, sizeof(line))) {
            buff_free(b);
            user_free(u);
            return NULL;
        }

        // Vérifie “nickname"
        if (strncmp(line, "nickname ", 9) != 0) {
            send(fd, "3 \r\n", 4, 0);
            continue;
        }

        // Extrait et nettoie CRLF
        char *nick = line + 9;
        size_t L = strlen(nick);
        if (L>=2 && nick[L-2]=='\r' && nick[L-1]=='\n') 
            nick[L-2]=0;
        else if (L>=1 && (nick[L-1]=='\n' || nick[L-1]=='\r')) 
            nick[L-1]=0;
        L = strlen(nick);

        //Vérifie longueur ≤16 et pas de ‘:’ 
        if (L==0 || L>MAX_NICKNAME_LEN || strchr(nick, ':')) {
            send(fd, "2 \r\n", 4, 0);
            continue;
        }

        //Vérifie unicité 
        int used = 0;
        pthread_mutex_lock(&list_lock);
        for (struct node *c = client_list->first; c; c = c->next) {
            struct user *o = c->elt;
            if (o != u && strcmp(o->nickname, nick) == 0) 
            {
                used = 1;
                break;
            }
        }
        pthread_mutex_unlock(&list_lock);
        if (used) 
        {
            send(fd, "1 \r\n", 4, 0);
            continue;
        }

        //sauvegarde et on ajoute l’utilisateur validé 
        strncpy(u->nickname, nick, MAX_NICKNAME_LEN);
        u->nickname[MAX_NICKNAME_LEN] = '\0';
        send(fd, "0 \r\n", 4, 0);

        //ajoute à la liste *après* validation 
        pthread_mutex_lock(&list_lock);
        list_add(client_list, u);
        pthread_mutex_unlock(&list_lock);
        break;
    }

    // Boucle de chat : préfixe nickname + message
    while (buff_fgets_crlf(b, line, sizeof(line))) 
    {
        char msg[1200];
        snprintf(msg, sizeof(msg), "%s: %s", u->nickname, line);
        write(pipefd[1], msg, strlen(msg));
    }

    // Déconnexion propre 
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &u->address.sin_addr, ip, sizeof(ip));

    printf("Client %s déconnecté (socket=%d, IP= %s, PORT= %d)\n",u->nickname, fd, ip, ntohs(u->address.sin_port));

    buff_free(b);
    pthread_mutex_lock(&list_lock);
    for (struct node *c = client_list->first; c; c = c->next) 
    {
        if (c->elt == u) {
            list_remove_node(client_list, c);
            break;
        }
    }
    pthread_mutex_unlock(&list_lock);
    user_free(u);
    return NULL;
}

int main(void) {
    client_list = list_create();
    int server_fd = create_listening_sock(PORT);
    if (server_fd < 0) 
    {
        fprintf(stderr, "Erreur de création du socket\n");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    pthread_t rep;
    pthread_create(&rep, NULL, repeater, NULL);
    pthread_detach(rep);

    printf("Serveur écoute sur le port %d\n", PORT);
    while (1) {
        struct user *u = user_accept(server_fd);
        if (!u) 
            continue;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &u->address.sin_addr, ip, sizeof(ip));

        printf("Nouvelle connexion: socket = %d, IP = %s, PORT = %d\n",u->sock, ip, ntohs(u->address.sin_port));

        int *pclient = malloc(sizeof(int));
        *pclient = u->sock;

        pthread_mutex_lock(&list_lock);
        list_add(client_list, pclient);
        pthread_mutex_unlock(&list_lock);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, u);
        pthread_detach(tid);
    }
    close(server_fd);
    return 0;
}
