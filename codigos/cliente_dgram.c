/**
 * Cliente DGRAM
 *
 * Servidor para atender peticiones que usan protocolo UDP, y sockets de
 * datagramas(DGRAM).
 *
 * Compilación: gcc servidor_dgram.c -Wall -o servidor_dgram
 *
 * @version 2.0 - 08/03/16
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

/**
 * Analiza los argumentos introducidos por linea de comandos al ejecutar el
 * programa.
 *
 * Para más información consultar 'man 3 getopt'.
 *
 * @param argc número de argumentos de entrada
 * @param argv arreglo de argumentos de entrada
 *
 * @return dirección ip ingresada en línea de comandos
 */
char* analizar_argumentos(int argc, char *argv[]) {
    static struct option opciones_largas[] = {
            {"destino", required_argument, 0, 'd'},
            {"help", no_argument, 0, 'h'},
            {"ipv4", no_argument, 0, '4'},
            {"ipv6", no_argument, 0, '6'},
            {0, 0, 0, 0}
        };
    int referencia = 0;  // variable usada para las opciones de los argumentos
    // evita se impriminan  mensajes 'default' de error en la terminal
    opterr = 0;
    int opcion;  // opción corta que se está leyendo al momento de usar la función

    char *ip_destino = NULL;

    // si un argumento es obligatorio se colocan dos puntos después de la
    // 'opción'(corta) elegida
    while ((opcion = getopt_long(argc, argv,"d:ha46",
                   opciones_largas, &referencia)) != -1) {
        switch (opcion) {
            case 'd':
                ip_destino = optarg;
                break;
            case 'a': case 'h':
                printf("\nModo de uso: %s [OPCIÓN]\n\n", argv[0]);
                printf("\t-d [IP], --destino [IP]\tDirección IP del destino");
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

    if(ip_destino == NULL) {
        fprintf(stderr, "\nFalta indicar la ip destino.\n");
        fprintf(stderr, "Usa %s --help para más información.\n\n",argv[0]);
        exit(1);
    }

    return ip_destino;
}


int main(int argc,  char *argv[]) {
    char *ip_destino = analizar_argumentos(argc, argv);
    printf("Se usará la familia de direcciones: '%s'\n\n",
        familia_direcciones == kIPV4 ? kMensajeIPV4 : kMensajeIPV6);

    struct addrinfo *info_destino = NULL;
    int descriptor = inicializar_cliente(ip_destino, kPuerto, SOCK_DGRAM,
        &info_destino);

    char *buffer = (char*)malloc(sizeof(char)*kMaxBuffer);

    while (strcmp(buffer, kMsjSalida) != 0) {
        scanf("%[^\n]s", buffer); // leo frase hasta que se pulsa salto de línea
        clear_buffer();
        enviar_datos_dgram(descriptor, info_destino, buffer, kMaxBuffer-1, 0);
    }

    printf("\nApagando cliente...\n");
    close(descriptor);

    return 0;
}
