
/*
 * `getaddrinfo` is not available in earlier versions of Windows.  The API
 * defaults to an old version of Windows, and you need to define WINVER to
 * be the oldest version of Windows you will support.  That setting enables
 * the functions that are available on newer versions of Windows.
 *
 * The constant values for setting WINVER to are defined in `w32api.h`
 */

#include <w32api.h>
// #define WINVER WindowsXP
#define WINVER WindowsVista

#include <ws2tcpip.h>
#include <winsock.h>
#include <stdio.h>

static struct addrinfo *
parse( char *address, char *service ) {
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        // .ai_socktype = SOCK_DGRAM,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0,
        // .ai_flags = (AI_NUMERICSERV | AI_ADDRCONFIG)
        .ai_flags = 0
    };

    struct addrinfo *info;
    int error = getaddrinfo( address, service, &hints, &info );

    switch ( error ) {
    case 0: break;
    case EAI_SERVICE:
        fprintf( stderr, "bad service '%s'\n", service );
        exit( -1 );
    case EAI_NONAME:
        fprintf( stderr, "bad address '%s'\n", address );
        exit( -1 );
    default:
        fprintf( stderr, "address parse error = %d\n", error );
        exit( -1 );
    }

    if ( info == NULL ) {
        fprintf( stderr, "cannot translate: %s:%s\n", address, service );
        exit( -1 );
    }

    return info;
}

static int
socket_open( char *address, char *port ) {
    struct addrinfo *local = parse( address, port );
    int sock = socket( local->ai_family, local->ai_socktype, local->ai_protocol );

    if ( sock == 0 ) {
        fprintf( stderr, "cannot open socket (%d)\n", GetLastError() );
        exit( -1 );
    }

    int error = bind( sock, local->ai_addr, local->ai_addrlen );
    if ( error < 0 ) {
        fprintf( stderr, "bind failed (%d)\n", GetLastError() );
        exit( -1 );
    }

    error = listen( sock, 5 );
    if ( error < 0 ) {
        fprintf( stderr, "listen failed (%d)\n", GetLastError() );
        exit( -1 );
    }

    return sock;
}

/*
 * Have to initialize winsock before making socket calls.
 *
 * http://stackoverflow.com/questions/11649259/winsock-send-fails-with-error-10093
 */
int main( int argc, char **argv ) { 
    /*
     * This *must* be called before any socket calls or they will fail and
     * GetLastError() will return 10093.
     */
    WSADATA data;
    int result = WSAStartup( MAKEWORD(2,2), &data );

    if ( result != 0 ) {
        fprintf( stderr, "winsock ini failed (%d)\n", GetLastError() );
        exit( -1 );
    }

    printf( "Parsing '%s':'%s'\n", argv[1], argv[2] );
    int sock = socket_open( argv[1], argv[2] );

    struct sockaddr peer;
    socklen_t length;

    int client = accept(sock, &peer, &length);

    if ( client < 0 ) {
        fprintf( stderr, "accept failed (%d)\n", GetLastError() );
        exit( -1 );
    }
}

/* vim: set autoindent expandtab sw=4 : */
