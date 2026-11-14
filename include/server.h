#ifndef SERVER_H
#define SERVER_H

/* Arranca el servidor en el puerto dado,
 * Solo vuelve si hay un error o si se recibe Ctrl+C. */
int server_run(int port, const char *root);

#endif
