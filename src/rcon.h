/*
 * src/rcon.h
 * (c) 2020 Jonas Gunz <himself@jonasgunz.de>
 * License: 
*/

/*
 * Length 	int 	Length of remainder of packet
 * Request ID 	int 	Client-generated ID
 * Type 	int 	3 for login, 2 to run a command, 0 for a multi-packet response
 * Payload 	byte[] 	ASCII text
 * 2-byte pad 	byte^2 	Two null bytes
 *
 * sizeof int: 4B
 * */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define RCON_LOGIN	3
#define RCON_LOLGINFAIL	-1
#define RCON_COMMAND	2
#define RCON_RESPONSE	0

#define RCON_RECV_PKGSIZE 4110
#define RCON_SEND_PKGSIZE 1460

typedef struct rcon_packet_s {
	int32_t length;
	int32_t id;
	int32_t type;
	char*	payload;
	uint32_t payload_len;
} rcon_packet_t;

int rcon_parse_packet ( rcon_packet_t* _packet, char* _buffer, uint32_t _len );

int rcon_construct_packet ( char* _buffer, uint32_t _len, rcon_packet_t* _packet );

char* rcon_strerror ( int _errno );
