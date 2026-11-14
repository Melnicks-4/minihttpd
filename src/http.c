#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "http.h"

static int copy_field(char *dst, size_t dst_size, const char *src, size_t n)
{
    if (n >= dst_size)
        return -1;
    memcpy(dst, src, n);
    dst[n] = '\0';
    return 0;
}

int http_parse_request(const char *buf, size_t len, struct http_request *req)
{
    memset(req, 0, sizeof(*req));

    /* Request */
    const char *line_end = memmem(buf, len, "\r\n", 2);
    if (line_end == NULL)
        return 400;
    size_t line_len = (size_t)(line_end - buf);

    const char *sp1 = memchr(buf, ' ', line_len);
    if (sp1 == NULL)
        return 400;
    const char *sp2 = memchr(sp1 + 1, ' ', line_len - (size_t)(sp1 + 1 - buf));
    if (sp2 == NULL)
        return 400;


    if (copy_field(req->method, sizeof(req->method), buf, (size_t)(sp1 - buf)) < 0)
        return 400;
    if (copy_field(req->uri, sizeof(req->uri), sp1 + 1, (size_t)(sp2 - sp1 - 1)) < 0)
        return 400;
    if (copy_field(req->version, sizeof(req->version), sp2 + 1,
                   (size_t)(line_end - sp2 - 1)) < 0)
        return 400;

    if (req->method[0] == '\0' || req->uri[0] != '/')
        return 400;
    for (const char *p = req->uri; *p; p++) {
        unsigned char ch = (unsigned char)*p;
        if (ch < 0x21 || ch == 0x7f)
            return 400;
    }
    if (strcmp(req->version, "HTTP/1.1") != 0 && strcmp(req->version, "HTTP/1.0") != 0)
        return 400;
    if (strcmp(req->method, "GET") != 0)
        return 405;

    /* Encabezados: Nombre: valor */
    const char *p = line_end + 2;
    const char *stop = buf + len;

    while (p < stop) {
        const char *eol = memmem(p, (size_t)(stop - p), "\r\n", 2);
        if (eol == NULL)
            return 400;
        if (eol == p)                  /* linea vacia: fin de encabezados */
            break;


        const char *colon = memchr(p, ':', (size_t)(eol - p));
        if (colon == NULL || colon == p)
            return 400;

        const char *val = colon + 1;
        size_t val_len = (size_t)(eol - val);
        while (val_len > 0 && (*val == ' ' || *val == '\t')) {
            val++;
            val_len--;
        }
        while (val_len > 0 && (val[val_len - 1] == ' ' || val[val_len - 1] == '\t'))
            val_len--;

        size_t name_len = (size_t)(colon - p);
        int rc = 0;
        if (name_len == 4 && strncasecmp(p, "Host", 4) == 0)
            rc = copy_field(req->host, sizeof(req->host), val, val_len);
        else if (name_len == 10 && strncasecmp(p, "Connection", 10) == 0)
            rc = copy_field(req->connection, sizeof(req->connection), val, val_len);
        else if (name_len == 10 && strncasecmp(p, "User-Agent", 10) == 0)
            rc = copy_field(req->user_agent, sizeof(req->user_agent), val, val_len);
        if (rc < 0)
            return 400;

        p = eol + 2;
    }

    /* Reglas de HTTP/1.1 */
    int is_11 = (strcmp(req->version, "HTTP/1.1") == 0);
    if (is_11 && req->host[0] == '\0')
        return 400;


    req->keep_alive = is_11;
    if (strcasecmp(req->connection, "close") == 0)
        req->keep_alive = 0;
    else if (strcasecmp(req->connection, "keep-alive") == 0)
        req->keep_alive = 1;

    return 0;
}

const char *http_status_text(int status)
{
    switch (status) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 500: return "Internal Server Error";
    default:  return "Unknown";
    }
}

int http_build_header(char *out, size_t out_size, int status,
                      const char *content_type, long length, int keep_alive)
{
    int n = snprintf(out, out_size,
        "HTTP/1.1 %d %s\r\n"
        "Server: MiniHTTPd/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "%s"
        "Connection: %s\r\n"

        "\r\n",
        status, http_status_text(status), content_type, length,
        (status == 405) ? "Allow: GET\r\n" : "",
        keep_alive ? "keep-alive" : "close");

    if (n < 0 || (size_t)n >= out_size)
        return -1;
    return n;
}
