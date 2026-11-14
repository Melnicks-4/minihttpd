#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define MAX_REQUEST 8192   /* tamano maximo de request line + encabezados */
#define MAX_URI     2048
#define MAX_VALUE   512

struct http_request {
    char method[16];
    char uri[MAX_URI];
    char version[16];
    char host[MAX_VALUE];
    char connection[64];
    char user_agent[MAX_VALUE];
    int  keep_alive;       /* 1 = mantener conexion, 0 = cerrar */
};

/* Analiza una peticion completa (termina en \r\n\r\n).
 * Devuelve 0 si esta bien, o el codigo de error (400 o 405). */
int http_parse_request(const char *buf, size_t len, struct http_request *req);

/* Texto del codigo: 200 "OK" */
const char *http_status_text(int status);

/* Arma la cabecera de respuesta */
int http_build_header(char *out, size_t out_size, int status,
                      const char *content_type, long length, int keep_alive);

#endif
