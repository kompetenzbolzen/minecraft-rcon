/*
 * src/rcon.c
 * (c) 2020 Jonas Gunz <himself@jonasgunz.de>
 * License: MIT
*/

/*
 * Length 	int 	Length of remainder of packet
 * Request ID 	int 	Client-generated ID
 * Type 	int 	3 for login, 2 to run a command, 0 for a multi-packet response
 * Payload 	byte[] 	ASCII text
 * 2-byte pad 	byte^2 	Two null bytes
 *
 * sizeof int: 4B
 * Little Endian
 *
 * LOGIN	3
 * LOGINFAIL	-1
 * COMMAND	2
 * RESPONSE	0
 */

#include "rcon.h"

static const unsigned int rcon_errors_len = 6;
static const char* rcon_errors[] = {
	"OK",
	"Undefined error",
	"Authentification failed",
	"Inadequate buffer",
	"Packet format error",
	"Connection timed out"
};

static int rcon_write_int ( char* _buffer, int _len, int32_t _data ) {
	if ( _len < 4 )
		return -1;

	for (int i = 0; i < 4; i++)
		_buffer[i] = (char)(_data >> (i*8));

	return 4;
}

static int rcon_read_int ( char* _buffer, int _len, int32_t* _out ) {
	if ( _len < 4 )
		return -1;

	*_out = 0;
	for (int i = 0; i < 4; i++)
		*_out |= (int32_t)((int32_t)_buffer[i] << (i*8));

	return 4;
}

int rcon_parse_packet ( rcon_packet_t* _packet, char* _buffer, uint32_t _len ) {
	if ( _len < 14 )
		return -1;

	int32_t bytecount = 0;
	
	bytecount += rcon_read_int ( _buffer + bytecount, _len - bytecount, &_packet->length );
	bytecount += rcon_read_int ( _buffer + bytecount, _len - bytecount, &_packet->id );
	bytecount += rcon_read_int ( _buffer + bytecount, _len - bytecount, &_packet->type );

	int32_t payload_len = _len - bytecount - 2;
	_packet->payload_len = payload_len;
	_packet->payload = malloc(payload_len);
	memcpy ( _packet->payload, _buffer + bytecount, payload_len );
	bytecount += payload_len;

	bytecount += 2; // padding
	//TODO check validity
	return bytecount;
}

int rcon_construct_packet ( char* _buffer, uint32_t _len, rcon_packet_t* _packet ) {
	if ( _len < RCON_SEND_PKGSIZE )
		return -1; // Payload does not fit.

	int32_t bytecount = 0;
	_packet->length = _packet->payload_len + 10;

	bytecount += rcon_write_int ( _buffer + bytecount, _len - bytecount, _packet->length );
	bytecount += rcon_write_int ( _buffer + bytecount, _len - bytecount, _packet->id );
	bytecount += rcon_write_int ( _buffer + bytecount, _len - bytecount, _packet->type );

	memcpy ( _buffer + bytecount, _packet->payload, _packet->payload_len );
	bytecount += _packet->payload_len;

	memcpy ( _buffer + bytecount, "\0\0", 2);
	bytecount += 2;

	return bytecount;
}

const char* rcon_strerror ( int _errno ) {
	return _errno >= rcon_errors_len ? rcon_errors[1] : rcon_errors[_errno];
}

// For packets created by rcon_parse_packet()
void rcon_free_packet ( rcon_packet_t* _packet ) {
	if ( ! _packet || ! _packet->payload )
		return;

	free ( _packet->payload );
	_packet->payload = NULL;
	_packet->payload_len = 0;
}

//returns 0 on success
int rcon_send_packet ( int _socket, rcon_packet_t* _packet ) {
	char buff [ RCON_SEND_PKGSIZE ];
	int ret;
	if ( _socket < 0 || !_packet )
		return RCON_ERR_GENERIC;

	if( ! ((ret = rcon_construct_packet ( buff, RCON_SEND_PKGSIZE, _packet )) > 0) )
		return RCON_ERR_PACKET;
	if ( send ( _socket, buff, ret, 0 ) > 0 )
		return 0;

	return RCON_ERR_GENERIC;
}

//returns 0 on success
int rcon_recv_packet ( int _socket, rcon_packet_t* _packet, int _timeout_msecs ) {
	char buff [ RCON_SEND_PKGSIZE ];
	int ret;
	struct pollfd fds;

	if ( _socket < 0 || !_packet )
		return RCON_ERR_GENERIC;

	fds.fd = _socket;
	fds.events = POLLIN;

	ret = poll ( &fds, 1, _timeout_msecs );
	if ( ret > 0 && fds.revents & POLLIN) {
		ret = recv ( _socket, buff, RCON_RECV_PKGSIZE, 0 );
		if ( ! (rcon_parse_packet ( _packet, buff, ret ) > 0) )
			return RCON_ERR_PACKET;

		return 0;
	}

	return RCON_ERR_TIMEOUT; //timeout
}

int rcon_login ( int _socket, const char* _password ) {
	int ret;
	rcon_packet_t result, request = {
		0,
		1337, // TODO remove hardcoded ID
		RCON_LOGIN,
		(char*) _password,
		strlen(_password)
	};

	if ( _socket < 0 || !_password )
		return RCON_ERR_GENERIC;

	if ( (ret = rcon_send_packet ( _socket, &request )) )
		return ret;

	if ( (ret = rcon_recv_packet ( _socket, &result, 1000 )) )
		return ret;

	if ( result.type != RCON_COMMAND || result.id != 1337 )
		return RCON_ERR_AUTH;

	return 0;
}

int rcon_command ( char** _output, int _socket, const char* _command ) {
	int ret;
	if ( _socket < 0 || !_command )
		return RCON_ERR_GENERIC;

	rcon_packet_t result, request = {
		0,
		1337, // TODO remove hardcoded ID
		RCON_COMMAND,
		(char*) _command, //NONONONONONONONONONONO
		strlen(_command)
	};

	if ( (ret = rcon_send_packet ( _socket, &request )) )
		return ret;

	if ( (ret = rcon_recv_packet ( _socket, &result, 1000 )) )
		return ret;

	// TODO truncated response
	*_output = malloc ( result.payload_len + 1 );
	*_output[result.payload_len] = 0;
	memcpy ( *_output, result.payload, result.payload_len );
	rcon_free_packet ( &result );

	if ( result.type != RCON_RESPONSE || result.id != 1337 )
		return RCON_ERR_PACKET;

	return 0;
}
