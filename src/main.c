/*
 * src/main.c
 * (c) 2020 Jonas Gunz <himself@jonasgunz.de>
 * License: 
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

int main( int argc, char* argv[] ) {
	program_params_t args = parse_args( argc, argv );

	int sock = -1;
	int ret = 0;
	struct addrinfo hints, *result, *iter;
	char recvbuf [ RCON_RECV_PKGSIZE ];
	char sendbuf [ RCON_SEND_PKGSIZE ];

	memset ( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo ( args.host, args.port, &hints, &result );
	if ( ret ) {
		fprintf ( stderr, "getaddrinfo: %s\n", gai_strerror(ret) );
		exit ( EXIT_FAILURE );
	}

	for ( iter = result; iter != NULL; iter = iter->ai_next ) {
		sock = socket( result->ai_family, result->ai_socktype, result->ai_protocol );
		if ( socket < 0 )
			continue;

		if ( connect ( sock, iter->ai_addr, iter->ai_addrlen ) != -1 )
			break;

		close (sock);
	}

	if ( ! iter ) {
		fprintf ( stderr, "Connection failed.\n" );
		exit ( EXIT_FAILURE );
	}

	fprintf ( stderr, "Connection successful.\n" );

	rcon_packet_t pack = {0,1337,RCON_LOGIN,args.password, strlen(args.password)}, rcon_result;
	ret = rcon_construct_packet ( sendbuf, RCON_SEND_PKGSIZE, &pack );

	ret = send ( sock, sendbuf, ret, 0 );

	ret = recv ( sock, recvbuf, RCON_RECV_PKGSIZE, 0);
	rcon_parse_packet ( &rcon_result, recvbuf, ret );

	if ( rcon_result.type != RCON_COMMAND ) {
		fprintf ( stderr, "Authentification failed." );
		exit ( EXIT_FAILURE );
	}

	rcon_packet_t pack2 = {0,6969,RCON_COMMAND, args.command, strlen(args.command)};

	ret = rcon_construct_packet ( sendbuf, RCON_SEND_PKGSIZE, &pack2 );

	ret = send ( sock, sendbuf, ret, 0 );

	ret = recv ( sock, recvbuf, RCON_RECV_PKGSIZE, 0);
	rcon_parse_packet ( &rcon_result, recvbuf, ret );

	printf("%s\n", rcon_result.payload);

	close (sock);

	return 0;
}

program_params_t parse_args ( int argc, char* argv[] ) {
	program_params_t ret = { "127.0.0.1", "25575", "1234", "list" };
	return ret;
}
