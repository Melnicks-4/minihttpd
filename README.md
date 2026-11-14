# minihttpd

Servidor HTTP/1.1 básico escrito en C para Linux, sin bibliotecas HTTP externas. Atiende a varios clientes con epoll, mantiene conexiones persistentes (keep-alive) y sirve archivos estáticos con el método GET.

Nombre: Melissa Guerra

# Estructura del repositorio
minihttpd/
├── Makefile
├── README.md
├── .gitignore
├── include/
│   ├── http.h
│   ├── server.h
│   ├── mime.h
│   └── files.h
├── src/
│   ├── main.c
│   ├── server.c
│   ├── http.c
│   ├── mime.c
│   └── files.c
└── www/
    ├── index.html
    ├── style.css
    └── image.png
