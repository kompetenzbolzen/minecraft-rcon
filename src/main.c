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

void print_help ( char* _progname );

int main( int argc, char* argv[] ) {
	program_params_t args;
	int ret, sock;
	char* result;

	args = parse_args( argc, argv );
	sock = connect_socket ( args.host, args.port );
	result = NULL;

	if ( (ret = rcon_login ( sock, args.password )) ) {
		fprintf ( stderr, "rcon_login: %s\n", rcon_strerror ( ret ) );
		exit ( EXIT_FAILURE );
	}

	if ( (ret = rcon_command ( &result, sock, args.command )) ) {
		fprintf ( stderr, "rcon_command: %s\n", rcon_strerror ( ret ) );
		exit ( EXIT_FAILURE );
	}

	printf("%s\n", result);

	close (sock);

	return 0;
}

program_params_t parse_args ( int argc, char* argv[] ) {
	program_params_t ret;
	memset(&ret, 0, sizeof ret);

	if ( argc < 5 )
		print_help ( argv[0] );

	ret.host = argv[1];
	ret.port = argv[2];
	ret.password = argv[3];

	unsigned int len = 0;
	for ( int i = 4; i < argc; i++ ) {
		len += strlen ( argv[i] ) + 1;
	}

	ret.command = malloc ( len + 1);
	ret.command[len] = 0;

	unsigned int cnt = 0;
	for ( int i = 4; i < argc; i++ ) {
		int arglen = strlen(argv[i]) + 1;
		memcpy ( ret.command + cnt, argv[i], arglen);
		(ret.command + cnt)[arglen - 1] = ' ';
		cnt += arglen;
	}
	ret.command[len-1] = 0; // Hacky AF

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

void print_help ( char* _progname ) {
	fprintf( stderr,
			"Minecraft RCON client\n"
			"Usage:\n"
			"	%s HOST PORT PASSWORD COMMAND ...",
			_progname
	      );
	exit ( EXIT_FAILURE );
}
