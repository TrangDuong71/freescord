/* Nom : DUONG THI THUY TRANG
 * Numero: 12306149
 * Je déclare qu'il s'agit de mon propre travail.
 * Ce travail a été réalisé intégralement par un être humain. */

#include "buffer.h"
#include <unistd.h>
#include <error.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct buffer {
    int fd;
    char *data;
    size_t size;
    size_t pos;
    size_t len;
    int eof;
};

buffer *buff_create(int fd, size_t buffsz)
{
    buffer *b = malloc(sizeof(buffer));
    if (b == NULL)
        error(2, 0, "malloc error - buff_create");
    b->fd = fd;
    b->data = malloc(buffsz);
    if (b->data == NULL)
        error(2, 0, "malloc error - buff_create");
    b->size = buffsz;
    b->pos = 0;
    b->len = 0;
    b->eof = 0;
    return b;
}

int buff_getc(buffer *b)
{
    if (b == NULL)
        return EOF;
    if (b->pos >= b->len) {
        ssize_t n = read(b->fd, b->data, b->size);
        if (n < 0) {
            error(1, errno, "read error in buff_getc");
        }
        if (n == 0) {
            b->eof = 1;
            return EOF;
        }
        b->len = n;
        b->pos = 0;
    }
    return (unsigned char)b->data[b->pos++];
}

int buff_ungetc(buffer *b, int c)
{
    if (b == NULL || c == EOF)
        return EOF;
    if (b->pos > 0) {
        b->pos--;
        /* Optionally overwrite the buffer char */
        b->data[b->pos] = (char)c;
    }
    return c;
}

void buff_free(buffer *b)
{
    if (b == NULL)
        return;
    free(b->data);
    free(b);
}

int buff_eof(const buffer *b)
{
    if (b == NULL)
        return 1;
    return b->eof;
}

int buff_ready(const buffer *b)
{
    if (b == NULL)
        return 0;
    return b->pos < b->len;
}

char *buff_fgets(buffer *b, char *dest, size_t size)
{
    if (b == NULL || dest == NULL || size == 0)
        return NULL;
    size_t i = 0;
    int c;
    while (i < size - 1) {
        c = buff_getc(b);
        if (c == EOF)
            break;
        dest[i++] = (char)c;
        if (c == '\n')
            break;
    }
    if (i == 0)
        return NULL;
    dest[i] = '\0';
    return dest;
}

char *buff_fgets_crlf(buffer *b, char *dest, size_t size) {
    if (b == NULL || dest == NULL || size == 0)
        return NULL;

    size_t i = 0;
    int c, prev=0;
    while (i < size - 1) {
        c = buff_getc(b);
        if (c == EOF) 
            break;

        dest[i++] = (char)c;

        if (prev == '\r' && c == '\n') {
            // terminez par \r\n
            break;
        }
        if (c == '\n') {
            break;
        }

        prev = c;
    }

    if (i == 0)
        return NULL;
    dest[i] = '\0';
    return dest;

}
