# minihttpd

Servidor HTTP/1.1 básico escrito en C para Linux, sin bibliotecas HTTP externas. Atiende a varios clientes con epoll, mantiene conexiones persistentes (keep-alive) y sirve archivos estáticos con el método GET.

Nombre: Melissa Guerra

## Estructura del repositorio
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

## Carpetas, archivos y contenido
`Makefile` Compila todos los .c de src/ en el ejecutable minihttpd.
`include/` Archivos de cabecera .h, declaran la interfaz pública de cada módulo.
`src/` Código fuente .c, implementación de cada módulo.
`www/` Contenido que el servidor entrega.

## Compilar
make

##Ejecutar en el puerto 8080 sirviendo la carpeta www/
./minihttpd

Ejecutar el servidor desde la raíz del proyecto para que la carpeta www/ se encuentre.
Después de debe abrir `http://localhost:8080/` en el navegador. 
Para detener el servidor, pulsar `Ctrl+C`: cierra todas las conexiones.

## Cómo funciona
El servidor solo recibe el nombre de un recurso y entrega los bytes del archivo correspondiente, indicando su tipo. Cuando el navegador abre `http://localhost:8080/`

## Códigos de estado 
200 - OK
400 - Bad Request 
403 - Forbidden
404 - Not Found 
405 - Method Not Allowed 
500 - Internal Server Error

## Pruebas
Con el servidor en marcha (`./minihttpd`), desde otra terminal:

Prueba: Comando - Resultado esperado
- Página principal: `curl -i http://localhost:8080/` - `200`, `Content-Type: text/html`
- CSS: `curl -i http://localhost:8080/style.css` - `200`, `text/css`
- Imagen: `curl -s -o /dev/null -w "%{http_code} %{content_type} %{size_download}\n" http://localhost:8080/image.png` - `200 image/png` y tamaño mayor que 0
- No existe: `curl -i http://localhost:8080/nada.html` - `404`
- Método inválido: `curl -i -X POST http://localhost:8080/` - `405` con `Allow: GET` 
- Traversal: `curl -i --path-as-is http://localhost:8080/../../../../../../etc/passwd` - `403` 
- Sin `Host`: `printf 'GET / HTTP/1.1\r\n\r\n' \| nc -w1 localhost 8080` - `400` 
- Petición mal formada: `printf 'NoPetición\r\n\r\n' \| nc -w1 localhost 8080` - `400` 
- URI enorme: `python3 -c "print('GET /'+'A'*10000+' HTTP/1.1\r\n\r\n', end='')" \| nc -w1 localhost 8080` - `400` (el cliente puede ver un *reset*; el servidor sigue vivo) 
- `Connection: close`: `curl -i -H "Connection: close" http://localhost:8080/` - Respuesta con `Connection: close` 
- Conexión persistente: `curl -v http://localhost:8080/ http://localhost:8080/style.css` - `Re-using existing connection`
- Concurrencia: Dejar `nc localhost 8080` abierto sin escribir y, en otra terminal, `curl http://localhost:8080/` - `curl` responde al instante
- Navegador: Abrir `http://localhost:8080/` - Se ve el título, el estilo y la imagen
 
