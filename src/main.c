/*
 * src/main.c
 * (c) 2020 Jonas Gunz <himself@jonasgunz.de>
 * License: MIT
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "rcon.h"

typedef struct program_params_s {
	char* 	host;
	char*	port;
	char*	password;
	char*	command;
} program_params_t;

program_params_t parse_args ( int argc, char* argv[] );

int connect_socket ( char* _host, char* _port );

int main( int argc, char* argv[] ) {
	program_params_t args = parse_args( argc, argv );

	int sock = connect_socket ( args.host, args.port );

	fprintf ( stderr, "Connection successful.\n" );

	if ( rcon_login ( sock, args.password ) ) {
		fprintf ( stderr, "Authentification failed.\n" );
		exit ( EXIT_FAILURE );
	}

	char* result = NULL;
        rcon_command ( &result, sock, args.command );

	printf("%s\n", result);

	close (sock);

	return 0;
}

program_params_t parse_args ( int argc, char* argv[] ) {
	program_params_t ret = { "127.0.0.1", "25575", "1234", "list" };
	return ret;
}

int connect_socket ( char* _host, char* _port ) {
	int sock = -1;
	int ret = 0;
	struct addrinfo hints, *result, *iter;

	memset ( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo ( _host, _port, &hints, &result );
	if ( ret ) {
		fprintf ( stderr, "getaddrinfo: %s\n", gai_strerror(ret) );
		exit ( EXIT_FAILURE );
	}

	for ( iter = result; iter != NULL; iter = iter->ai_next ) {
		sock = socket( result->ai_family, result->ai_socktype, result->ai_protocol );
		if ( sock < 0 )
			continue;

		if ( connect ( sock, iter->ai_addr, iter->ai_addrlen ) != -1 )
			break;

		close (sock);
	}

	if ( ! iter ) {
		fprintf ( stderr, "Connection failed.\n" );
		exit ( EXIT_FAILURE );
	}

	return sock;
}

