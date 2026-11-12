#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main(void){

	// Direcciones IPv4 y TCP
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0){
		perror("socket");
		return 1;
	}

	int opt = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    		perror("bind");
    		return 1;
	}

	if (listen(listen_fd, 128) < 0) {
    		perror("listen");
    		return 1;
	}

	printf("Escuchando en puerto %d\n", PORT);

	for (;;) {
 	   int client_fd = accept(listen_fd, NULL, NULL);
	   if (client_fd < 0) {
		perror("accept");
        	continue;
    	   }

    	char buf[4096];
    	ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    	if (n > 0) {
        	buf[n] = '\0';
        	printf("--- peticion ---\n%s", buf);
    	}

    	const char *body = "Hola desde MiniHTTPd\n";
    	char resp[512];
    	int len = snprintf(resp, sizeof(resp),
        	"HTTP/1.1 200 OK\r\n"
        	"Content-Type: text/plain\r\n"
        	"Content-Length: %zu\r\n"
        	"Connection: close\r\n"
        	"\r\n"
        	"%s",
        	strlen(body), body);

    	write(client_fd, resp, len);
    	close(client_fd);
	}

	return 0;
}
