# minihttpd

Servidor HTTP/1.1 básico escrito en C para Linux, sin bibliotecas HTTP externas. Atiende a varios clientes con epoll, mantiene conexiones persistentes (keep-alive) y sirve archivos estáticos con el método GET.

Nombre: Melissa Guerra

# Estructura del repositorio
```
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
```

# Compilar
make
Ejecutar en el puerto 8080 sirviendo la carpeta www/
./minihttpd

# Cómo funciona
El servidor solo recibe el nombre de un recurso y entrega los bytes del archivo correspondiente, indicando su tipo. Cuando el navegador abre `http://localhost:8080/`

