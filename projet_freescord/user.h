/* Nom : DUONG THI THUY TRANG
 * Numero: 12306149
 * Je déclare qu'il s'agit de mon propre travail.
 * Ce travail a été réalisé intégralement par un être humain. */

#ifndef USER_H
#define USER_H

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_NICKNAME_LEN 17

struct user {
    struct sockaddr_in address;
    socklen_t addr_len;
    int sock;
    char nickname[MAX_NICKNAME_LEN];
};

struct user *user_accept(int sl);
void user_free(struct user *user);

#endif /* ifndef USER_H */
