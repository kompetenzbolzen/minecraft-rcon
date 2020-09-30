/*
 * src/rcon.c
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
 * Little Endian
 *
 * LOGIN	3
 * LOGINFAIL	-1
 * COMMAND	2
 * RESPONSE	0
 */

#include "rcon.h"

static char* rcon_errors[] = {
	"No Error.",
	"Undefined Error.",
	"Login Denied.",
	"Buffer size incorrect.",
	"Malformed packet recieved."
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

char* rcon_strerror ( int _errno ) {
	return _errno > 4 ? rcon_errors[1] : rcon_errors[_errno];
}
