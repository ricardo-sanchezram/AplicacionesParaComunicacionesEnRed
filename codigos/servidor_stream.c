/**
 * Servidor STREAM
 *
 * Servidor para atender peticiones que usan protocolo TCP, y sockets de
 * datagramas(STREAM).
 *
 * Compilación: gcc servidor_stream.c -Wall -o servidor_stream
 *
 * @version 2.0 - 03/04/16
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // memset
#include <unistd.h>  // 'getopt()'
#include <getopt.h>  // 'getopt()'

#include "funciones_sockets.h"

// constantes
const char *kPuerto = "6666";  // puerto de servicio
const int kMaxBuffer = 100;
const char *kMsjSalida = "exit"; // Mensaje para salir del programa
const int kMaxConexiones = 10; // Mensaje para salir del programa

/**
 * Analiza los argumentos introducidos por linea de comandos al ejecutar el
 * programa.
 *
 * Para más información consultar 'man 3 getopt'.
 *
 * @param argc número de argumentos de entrada
 * @param argv arreglo de argumentos de entrada
 */
void analizar_argumentos(int argc, char *argv[]) {
    static struct option opciones_largas[] = {
            {"help", no_argument, 0, 'h'},
            {"ipv4", no_argument, 0, '4'},
            {"ipv6", no_argument, 0, '6'},
            {0, 0, 0, 0}
        };
    int referencia = 0;  // variable usada para las opciones de los argumentos
    // evita se impriminan  mensajes 'default' de error en la terminal
    opterr = 0;
    int opcion;  // opción corta que se está leyendo al momento de usar la función

    // si un argumento es obligatorio se colocan dos puntos después de la
    // 'opción'(corta) elegida
    while ((opcion = getopt_long(argc, argv,"ha46",
                   opciones_largas, &referencia)) != -1) {
        switch (opcion) {
            case 'a': case 'h':
                printf("\nModo de uso: %s [OPCIÓN]\n\n", argv[0]);
                //printf("\t-d [IP], --destino [IP]\tDirección IP del destino");
                printf("(IPv4 o IPv6)\n");
                printf("\t-h --help\tLista de ayuda y opciones\n");
                printf("\t-4, --ipv4\tUsar direcciones de tipo IPv4\n");
                printf("\t-6, --ipv6\tUsar direcciones de tipo IPv6\n");
                printf("\nNOTA: Si no se especifica tipo de dirección se usará");
                printf(" 'IPv4'(--ipv4) por defecto\n\n");
                exit(0);
            case '4':
                familia_direcciones =  kIPV4;
                break;
            case '6':
                familia_direcciones =  kIPV6;
                break;
            case '?': default: // si hay una opción no registrada o regresa 0
                fprintf(stderr, "\nOpción inválida: -- %c\n", optopt);
                fprintf(stderr, "Usa %s --help para más información.\n\n",argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }
}


int main(int argc,  char *argv[]) {
    analizar_argumentos(argc, argv);
    printf("Se usará la familia de direcciones: '%s'\n\n",
        familia_direcciones == kIPV4? kMensajeIPV4 : kMensajeIPV6);

    int descriptor = inicializar_servidor(kPuerto, SOCK_STREAM);

    escuchar(descriptor, kMaxConexiones);



    char *buffer = (char*)malloc(sizeof(char)*kMaxBuffer);

    struct sockaddr_storage cliente;
    int bytes_recibidos = 0;

    // debe implementarse como hilo

    int descriptor_cliente = aceptar(descriptor, (struct sockaddr*)&cliente);

    while (strcmp(buffer, kMsjSalida) != 0) {
        bytes_recibidos = recibir_datos_stream(descriptor_cliente, buffer,
            kMaxBuffer-1, 0);
        buffer[bytes_recibidos] = '\0';
        printf("-------------------------------------------------\n");
        printf("%d datos recibidos de %s\n", bytes_recibidos,
            obtener_direccion_imprimible((struct sockaddr*)&cliente));
        printf("El mensaje es: \"%s\"\n", buffer);
    }

    close(descriptor_cliente);

    // termina proceso hilo

    printf("\nApagando servidor...\n");
    close(descriptor);

    return 0;
}
