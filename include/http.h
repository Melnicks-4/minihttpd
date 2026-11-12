#ifndef HTTP_H
#define HTTP_H

#define MAX_REQUEST 8192
#define MAX_URI 2048

int http_handle_request(int client_fd, const char *request);

#endif
