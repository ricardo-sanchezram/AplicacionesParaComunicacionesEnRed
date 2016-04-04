/**
 * Funciones para sockets
 *
 * Contiene funciones para la comunicación entre host/servidor, usando sockets
 * de datagramas(SOCK_DGRAM) y sockets de flujo(SOCK_STREAM).
 *
 * Notas:
 * - Para especificar la familia de direcciones a utilizar, se indica AF_INET
 * 	para IPv4 o AF_INET6 para IPv6
 * - Para construir esta libería se usó como referencia el documento:
 * "Beej's Guide to Network Programming"
 *
 *
 * @version 2.0 - 03/04/16
 */

#ifndef FUNCIONES_SOCKETS_H_
#define FUNCIONES_SOCKETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>  // variable error
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  // nuevas funciones e IPv6
#include <netdb.h>  // para 'getaddrinfo()'
#include <unistd.h>  // para 'close()'


// 'mensaje' usado para mostrar en salida el tipo de dirección
const char *kMensajeIPV4 = "IPv4";
const char *kMensajeIPV6 = "IPv6";

// 'códigos' que indican la familia de direcciones a usar para la comunicación
typedef enum {kIPV4, kIPV6} Familia_direcciones;
// familia de direcciones que se usará por defecto
Familia_direcciones familia_direcciones =  kIPV4;

/* Prototipos */

extern inline void clear_buffer();
void* extraer_direccion_sockaddr(const struct sockaddr *sa);
const char* obtener_direccion_imprimible(const struct sockaddr *sa);
struct addrinfo* crear_estructura_referencia(int tipo_socket);
struct addrinfo* obtener_direccion(const char *ip_host, const char *puerto,
        const struct addrinfo *referencia);
int crear_socket(const struct addrinfo *info_direccion);
int asociar_socket(int descriptor, const struct addrinfo *info_direccion);
int inicializar_cliente(const char *ip_destino, const char *puerto,
        int tipo_socket, struct addrinfo **info_destino);
int inicializar_servidor(const char *puerto, int tipo_socket);
int recibir_datos_dgram(int descriptor, char *buffer, int tam_buffer, int bandera,
       struct sockaddr *info_origen);
int enviar_datos_dgram(int descriptor, struct addrinfo *info_destino,
       char *buffer, int tam_buffer, int bandera);
int conectar(int descriptor, struct addrinfo *info_direccion);
int escuchar(int descriptor, int reserva);
int aceptar(int descriptor, struct sockaddr *info_origen);
int enviar_datos_stream(int descriptor, char *buffer, int tam_buffer,
      int bandera);
int recibir_datos_stream(int descriptor, char *buffer, int tam_buffer,
      int bandera);

/* Funciones */

// limpia el buffer de entrada: filtra el salto de linea al capturar datos
inline void clear_buffer() {while ((getchar() != '\n') && (getchar() != EOF));}

/**
 * Extrae dirección ip de una estructura sockaddr.
 *
 * Identifica el tipo de dirección usado(IPv4 o IPv6) y hace un cast a
 * 'sockaddr_in' ó 'sockaddr_in6' según corresponda, para obtener una estructura
 * 'in_addr' o 'in6_addr' según la familia, la cual contiene su dirección.
 *
 * @param sa estructura sockaddr con la información de la dirección
 *
 * @return estructura 'in_addr' o 'in6_addr'
 */
void* extraer_direccion_sockaddr(const struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 * Obtiene la dirección ip en formato imprimible a partir de una estructura
 * 'sockaddr'.
 *
 * Crea una cadena del tamaño de una dirección IPv6(para poder almacenar
 * IPv4 o IPV6), y usa la función 'inet_ntop()'(network to presentation) para
 * obtener la dirección en una formato imprimible con su correspondiente
 * versión(IPv4 o IPv6).
 *
 * Para más información consultar 'man 3 inet_ntop'.
 *
 * @param sa estructura sockaddr con la información de la dirección
 *
 * @return cadena con la dirección ip
 */
const char* obtener_direccion_imprimible(const struct sockaddr *sa) {
    char *ip = (char*)malloc(INET6_ADDRSTRLEN);
    inet_ntop(sa->sa_family, extraer_direccion_sockaddr(&(*sa)), ip,
        INET6_ADDRSTRLEN);
    return ip;
}

/**
 * Crea una estructura 'addrinfo', la cuál servirá como parámetro para la
 * función 'gettaddrinfo()'.
 *
 * El objetivo de esta estructura es especificar ciertos campos en ella que
 * servirán como "restricciones" al momento de obtener las direcciones(propias o
 * las del host destino).
 *
 * Para más información consultar 'man 3 gettaddrinfo'.
 *
 * @param tipo_socket socket que se usará: SOCK_STREAM o SOCK_DGRAM
 *
 * @return apuntador a estructura addrinfo con "parámetros" establecidos
 */
struct addrinfo* crear_estructura_referencia(int tipo_socket) {
    struct addrinfo *referencia = (struct addrinfo*)malloc(sizeof(struct addrinfo));

    memset(referencia, 0, sizeof(struct addrinfo));
    // es posible especificar 'AF_UNSPEC' para obtener direcciones IPv4 y/o IPv6
    if (familia_direcciones == kIPV4) {
        referencia->ai_family = AF_INET;
    } else {
        referencia->ai_family = AF_INET6;
    }
    referencia->ai_socktype = tipo_socket;
    // Se especifica la bandera 'AI_PASSIVE', entonces se eligirá la dirección
    // del host, usualmente se usa esta opción cuando se elige funcionar como
    // servidor. Establece  la bandera 'INADDR_ANY' en la estructura sockaddr_in
    // o 'IN6ADDR_ANY_INIT' si la estructura es sockaddr_in6.
    // Esta bandera será ignorada en caso de fungir como cliente y que al llamar
    // la función 'getaddrinfo()' se indique una ip para un host.
    referencia->ai_flags = AI_PASSIVE;

    return referencia;
}

/**
 * Usa la función 'gettaddrinfo()' para obtener las direcciones que se ajusten
 * a las restricciones que se indicaron en la estructura de referencia.
 * Estás direcciones pueden ser propias si se elige fungir como servidor o
 * externas si se elige fungir como cliente.
 *
 * Para más información consultar 'man 3 gettaddrinfo'.
 *
 * @param  ip_host IP o "nombre" del host con el que se establecerá comunicación.
 *                 En caso de que se especifique el nombre, se pregunta
 *                 automáticamente al DNS por la dirección de este.
 *                 En caso de fungir como un servidor se indica 'NULL' para que
 *                 sea usada la dirección propia.
 * @param  puerto cadena con el número o nombre(servicio) de puerto en donde se
 *                conectará(si se funge como cliente) o brindará servicio(si se
 *                funge como servidor).
 *                Si se especifica como "0" tomará un puerto aleatorio disponible.
 * @param  referencia estructura 'addrinfo' la cual debió de ser llenada
 *                    previamente con las restricciones para la búsqueda de
 *                    direcciones. Para este parámetro puede usarse la función
 *                    'crear_estructura_referencia()'
 *                    Es posible especificar como 'NULL' (consulte manual).
 *
 * @return estructura addrinfo que contiene un lista de direcciones que se hayan
 *                    ajustado a los parámetros especificados.
 */
struct addrinfo* obtener_direccion(const char *ip_host, const char *puerto,
        const struct addrinfo *referencia) {
    struct addrinfo *direcciones;
    int res;
    if ((res = getaddrinfo(ip_host, puerto, referencia, &direcciones)) != 0) {
        fprintf(stderr, "\nError al obtener información con getaddrinfo: %s\n",
            gai_strerror(res));
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } /*
    if (direcciones == NULL) {
        fprintf(stderr,"\nNo se pudo obtener el tipo de dirección solicitado\n");
        freeaddrinfo(direcciones);
        exit(EXIT_FAILURE);
    }*/

    return direcciones;
}

/**
 * Crea un socket con la información especificada en la estructura de dirección
 * de tipo 'addrinfo' la cuál debe ser completada previamente con la función
 * 'getaddinfo()' o la función propia 'obtener_direccion()'
 *
 * Para más información consulte 'man socket'.
 *
 * @param info_direccion estructura con la información para crear el socket
 *
 * @return descriptor identificador del socket creado
 */
int crear_socket(const struct addrinfo *info_direccion) {
    int descriptor = socket(info_direccion->ai_family,
        info_direccion->ai_socktype, info_direccion->ai_protocol);
    if (descriptor == -1) {
        fprintf(stderr,"\nError al crear socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return descriptor;
}

/**
 * Asocia un socket por medio de su descriptor a una dirección y a un puerto.
 * Esto se realiza usualmente si se desea atender peticiones en cierta dirección
 * y puerto(fungir como servidor).
 *
 * Para más información consulte 'man bind'.
 *
 * @param descriptor identificador del socket
 * @param info_direccion estructura con la información de la dirección
 *
 * return valor que regresa la función 'bind'
 */
int asociar_socket(int descriptor, const struct addrinfo *info_direccion) {
    int valor_retorno = bind(descriptor, info_direccion->ai_addr,
        info_direccion->ai_addrlen);
    if (valor_retorno == -1) {
        close(descriptor);
        fprintf(stderr,"\nError al asociar(bind) socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return valor_retorno;
}

/**
 * Inicializa un host como un servidor, que escuchará en el puerto indicado.
 *
 * Se crea un socket del tipo indicado(SOCK_STREAM o SOCK_DGRAM) el cual se
 * asocia con la dirección propia y con el puerto indicado para poder fungir
 * como servidor.
 *
 * @param  puerto que se asociará al socket de la dirección. Por lo tanto será
 *                el puerto donde se brindará servicio.
 * @param tipo_socket tipo de socket a usar para la comunicación
 *
 * @return descriptor del socket
 */
int inicializar_servidor(const char *puerto, int tipo_socket) {

    struct addrinfo *info_servidor; // guardará mi información como servidor

    // se indica 'NULL' ya que no se comunicará con un dirección en específico
    // y se usará la propia dirección para brindar servicio
    info_servidor = obtener_direccion(NULL, puerto,
            crear_estructura_referencia(tipo_socket));

    int descriptor = crear_socket(info_servidor);

    // permite reutilizar el puerto y dirección
    int yes=1;
    if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
            == -1) {
        close(descriptor);
        fprintf(stderr,"\nError al establecer operación(setsockopt) a socket: %s\n",
            strerror(errno));
        exit(EXIT_FAILURE);
    }

    asociar_socket(descriptor, info_servidor);

    freeaddrinfo(info_servidor);
    return descriptor;
}

/**
* Inicializar el host como un cliente para comunicarse a otro host destino
* (usualmente un servidor).
*
* Obtiene la dirección del host destino y se crear un socket del tipo indicado
* (SOCK_STREAM o SOCK_DGRAM) con la información.
*
* @param ip_destino dirección del host/servidor con el que se comunicará
* @param puerto en donde se comunicará con el nuevo host
* @param tipo_socket tipo de socket a usar para la comunicación
* @param info_destino Estructura donde se almacenará la información del destino
*                     (dirección)
*
* @return descriptor del socket
*/
int inicializar_cliente(const char *ip_destino, const char *puerto,
        int tipo_socket, struct addrinfo **info_destino) {
    *info_destino = obtener_direccion(ip_destino, puerto,
        crear_estructura_referencia(tipo_socket));
    int descriptor = crear_socket(*info_destino);

    return descriptor;
}

// ---------------------------------------------------------
// STREAM - Funciones para sockets de flujo
// ---------------------------------------------------------

/**
 * Establece conexión con un host remoto o servidor.
 *
 * Para más información consulte 'man connect'.
 *
 * @param descriptor identificador del socket
 * @param info_direccion estructura con la información de la dirección
 *
 * return valor que regresa la función 'connect'
 */
 int conectar(int descriptor, struct addrinfo *info_direccion) {
     int valor_retorno = connect(descriptor, info_direccion->ai_addr,
        info_direccion->ai_addrlen);
     if (valor_retorno == -1) {
         close(descriptor);
         fprintf(stderr,"\nError al conectar(connect): %s\n",
            strerror(errno));
         exit(EXIT_FAILURE);
     }

     return valor_retorno;
 }

/**
 * Prepara al socket para escuchar peticiones de clientes y formarlas en una
 * cola.
 *
 * Para más información consulte 'man listen'.
 *
 * @param descriptor identificador del socket a asociar
 * @param reserva el número máximo de conexiones en la cola de espera.
 *                (Varios sistemas limitan este número a 20)
 *
 * return valor que regresa la función 'listen'
 */
int escuchar(int descriptor, int reserva) {
    int valor_retorno = listen(descriptor, reserva);
    if (valor_retorno == -1) {
        close(descriptor);
        fprintf(stderr,"\nError al escuchar(connect) con socket: %s\n",
            strerror(errno));
        exit(EXIT_FAILURE);
    }

    return valor_retorno;
}

/**
 * Toma una conexión pendiente de la cola de espera(al usar 'listen()') y la
 * acepta.
 *
 * Llama a la función 'accept()' para aceptar la conexión pendiente y regrese
 * un nuevo descriptor de socket para usar las funciones 'send()' y 'recv()'
 * con esta conexión específica.
 * El socket original permanece escuchando más peticiones.
 *
 * Para más información consulte 'man accept'.
 *
 * @param descriptor identificador del socket que escucha('listen()')
 * @param info_origen estructura donde se guarda la información de la conexión
 *                    entrante(cliente). De preferencia deberá ser una estructura
 *                    'sockaddr_storage', la cuál sirve para ipv4 e ipv6
 *
 * return descriptor del nuevo socket creado para la conexión entrante(cliente)
 */
int aceptar(int descriptor, struct sockaddr *info_origen) {
    socklen_t tam_dir = sizeof(struct sockaddr_storage);  // tamaño para ipv4,ipv6
    int descriptor_cliente = accept(descriptor, info_origen, &tam_dir);
    if (descriptor_cliente == -1) {
        close(descriptor);
        fprintf(stderr,"\nError al aceptar(accept) conexión: %s\n",
            strerror(errno));
        exit(EXIT_FAILURE);
    }

    return descriptor_cliente;
}

/**
 * Envia la cadena especificada a la dirección y puerto que relacionados con el
 * descriptor de socket.
 *
 * Para más información consulte 'man send'.
 *
 * @param descriptor identificador del socket abierto
 * @param buffer cadena de bytes a enviar
 * @param tam_buffer tamaño del buffer
 * @param bandera opción para indicar cierta funcionalidad al socket para
 *                el envio de los datos. Usualmente es 0
 *
 * @return número de bytes enviados
 */
int enviar_datos_stream(int descriptor, char *buffer, int tam_buffer,
        int bandera) {
    int bytes_enviados = send(descriptor, buffer, tam_buffer, bandera);
    if (bytes_enviados == -1) {
        fprintf(stderr, "\nError al enviar datos(send): %s\n", strerror(errno));
    }

    return bytes_enviados;
}

/**
 * Recibe la trama entrante, y almacena los datos en el buffer especificado.
 *
 * Para más información consulte 'man recv'.
 *
 * @param descriptor identificador del socket abierto
 * @param buffer variable donde se guardará los bytes entrantes (esta ya debe
 *               tener memoria reservada)
 * @param tam_buffer tamaño del buffer
 * @param bandera opción para indicar cierta funcionalidad al socket para
 *                recibir los datos. Usualmente es 0
 *
 * @return número de bytes recibidos
 */
int recibir_datos_stream(int descriptor, char *buffer, int tam_buffer,
        int bandera) {
    int bytes_recibidos = recv(descriptor, buffer, tam_buffer, bandera);
    if (bytes_recibidos == -1) {
        if(bandera != MSG_DONTWAIT) { // si es socket bloqueante
            fprintf(stderr,"\nError al recibir datos(recvfrom): %s\n",
                strerror(errno));
        }
        // si se configura la función como socket NO bloqueante la función
        // regresa -1 cada vez que no escucha nada
    }

    return bytes_recibidos;
}



// ---------------------------------------------------------
// DGRAM - Funciones para sockets de datagramas
// ---------------------------------------------------------

/**
 * Recibe las trama entrante, almacena los datos y la información de quien lo
 * envía.
 *
 * Nota: La estructura donde se guarda la información del cliente es una
 * 'sockaddr', por lo tanto es posible usar una estructura equivalente como son:
 * 'sockaddr_in', 'sockaddr_in6' o 'sockaddr_storage'.
 *
 * Para más información consulte 'man recvfrom'.
 *
 * @param descriptor identificador del socket abierto
 * @param buffer variable donde se guardará los bytes entrantes (esta ya debe
 *               tener memoria reservada)
 * @param tam_buffer tamaño del buffer
 * @param bandera opción para indicar cierta funcionalidad al socket para
 *                recibir los datos. Usualmente es 0
 * @param info_origen estructura donde se guarda la información del quien envia
 *                    la información. De preferencia deberá ser una estructura
 *                    'sockaddr_storage', la cuál sirve para ipv4 e ipv6
 *
 * @return número de bytes recibidos
 */
int recibir_datos_dgram(int descriptor, char *buffer, int tam_buffer, int bandera,
        struct sockaddr *info_origen) {
    socklen_t tam_dir = sizeof(struct sockaddr_storage);  // tamaño para ipv4, ipv6
    int bytes_recibidos = recvfrom(descriptor, buffer, tam_buffer, bandera,
            info_origen, &tam_dir);
    if (bytes_recibidos == -1) {
        if(bandera != MSG_DONTWAIT) { // si es socket bloqueante
            fprintf(stderr,"\nError al recibir datos(recvfrom): %s\n",
                strerror(errno));
        }
        // si se configura la función como socket NO bloqueante la función
        // regresa -1 cada vez que no escucha nada
    }

    return bytes_recibidos;
}

/**
 * Envia la cadena especificada a la dirección y puerto que contenga la
 * estructura 'addrinfo'.
 *
 * Para más información consulte 'man sendto'.
 *
 * @param descriptor identificador del socket abierto
 * @param info_destino estructura que contiene la información de a donde se
 *                      enviarán los datos
 * @param buffer cadena de bytes a enviar
 * @param tam_buffer tamaño del buffer
 * @param bandera opción para indicar cierta funcionalidad al socket para
 *                el envio de los datos. Usualmente es 0
 *
 * @return número de bytes enviados
 */
int enviar_datos_dgram(int descriptor, struct addrinfo *info_destino,
        char *buffer, int tam_buffer, int bandera) {
    int bytes_enviados = sendto(descriptor, buffer, tam_buffer, bandera,
            info_destino->ai_addr, info_destino->ai_addrlen);
    if (bytes_enviados == -1) {
        fprintf(stderr, "\nError al enviar datos(sendto): %s\n", strerror(errno));
    }

    return bytes_enviados;
}


#endif  // FUNCIONES_SOCKETS_H_

/*
Estructuras usadas para sockets:

// usada para la función 'getaddrinfo'
struct addrinfo {
               int              ai_flags;  // AI_PASSIVE, AI_CANONNAME, etc.
               int              ai_family;  // AF_INET, AF_INET6, AF_UNSPEC
               int              ai_socktype;  // SOCK_DGRAM, SOCK_STREAM
               int              ai_protocol;  // usa '0' para cualquiera
               socklen_t        ai_addrlen;  // tamaño de 'ai_addr' en bytes
               struct sockaddr *ai_addr;  // struct sockaddr_in o sockaddr_in6
               char            *ai_canonname;  // nombre canónico completo
               struct addrinfo *ai_next;  // siguiente nodo de lista ligada
           };

// contiene la familia, dirección y puerto
struct sockaddr {
    unsigned short    sa_family;    // familia de direcciones
    char              sa_data[14];  // 14 bytes de dirección de protocolo
};

// IPv4 AF_INET sockets:

// equivalente a sockaddr
struct sockaddr_in {
    short            sin_family;   // AF_INET, AF_INET6
    unsigned short   sin_port;     // htons(3490), puerto
    struct in_addr   sin_addr;     // estructura con dirección
    char             sin_zero[8];  // zero this if you want to
};

struct in_addr {
    unsigned long s_addr;  // dirección IPv4, llenar con 'inet_pton()'
};


// IPv6 AF_INET6 sockets:

// equivalente a sockaddr
struct sockaddr_in6 {
    u_int16_t       sin6_family;   // AF_INET6
    u_int16_t       sin6_port;     // número puerto, Network Byte Order
    u_int32_t       sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // estructura con dirección
    u_int32_t       sin6_scope_id; // Scope ID
};

struct in6_addr {
    unsigned char   s6_addr[16];  // dirección IPv6, llenar con 'inet_pton()'
};

// estrutura general para direcciones de socket para almacenar información de
// una estructura 'sockaddr_in' o 'sockaddr_in6'.
struct sockaddr_storage {
    sa_family_t  ss_family;     // familia de direcciones
    // all this is padding, implementation specific
    char      __ss_pad1[_SS_PAD1SIZE];
    int64_t   __ss_align;
    char      __ss_pad2[_SS_PAD2SIZE];
};


*/
