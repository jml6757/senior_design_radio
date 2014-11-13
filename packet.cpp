#include <string.h> /* Used for memory copies */

#include "packet.h"

/******************************************************************************
 * Creates a data packet to send using a predefined buffer. The buffer must
 * made be large enough to hold the entire packet contents.
 *
 * @param     buf     The buffer to insert the packet into
 * @param     data    The data to send
 * @param     size    The size of the data
 * @param     seqnum  The sequence number of the data packet
 * @param     total   The total number of data packets that will be sent
 *
 * @return    The size of the packet generated
 *****************************************************************************/
int packet_data_send_create(uint8_t* buf, uint8_t* data, uint16_t size, uint16_t seqnum, uint16_t total)
{
	struct packet* packet = (struct packet*) buf;

	packet->type = ITP_TYPE_DATA_SEND;
	packet->seqnum = seqnum;
	packet->total = total;
	packet->size = size;

	memcpy(&packet->data, data, size);

	packet->crc = packet_generate_crc(packet);

	return sizeof(struct packet) - 1 + size;
}

/******************************************************************************
 * Creates an ack message using a predefined buffer. The buffer must
 * made be large enough to hold the entire packet contents.
 *
 * @param     buf     The buffer to insert the packet into
 * @param     seqnum  The sequence number of the data packet that was recieved
 * @param     total   The total number of the recieved packet
 *
 * @return    The size of the packet generated
 *****************************************************************************/
int packet_data_ack_create(uint8_t* buf, uint16_t seqnum, uint16_t total)
{
	struct packet* packet = (struct packet*) buf;

	packet->type = ITP_TYPE_DATA_ACK;
	packet->seqnum = seqnum;
	packet->total = total;
	packet->size = 0;
	packet->data = 0;
	packet->crc = packet_generate_crc(packet);
	
	return sizeof(struct packet);
}

/******************************************************************************
 * Creates a nack message using a predefined buffer. The buffer must
 * made be large enough to hold the entire packet contents.
 *
 * @param     buf     The buffer to insert the packet into
 * @param     seqnum  The sequence number of the data packet that was recieved
 * @param     total   The total number of the recieved packet
 *
 * @return    The size of the packet generated
 *****************************************************************************/
int packet_data_nack_create(uint8_t* buf, uint16_t seqnum, uint16_t total)
{
	struct packet* packet = (struct packet*) buf;

	packet->type = ITP_TYPE_DATA_NACK;
	packet->seqnum = seqnum;
	packet->total = total;
	packet->size = 0;
	packet->data = 0;
	packet->crc = packet_generate_crc(packet);

	return sizeof(struct packet);
}

/******************************************************************************
 * Creates an error message for when data reception is irrecoverable. The 
 * buffer must made be large enough to hold the entire packet contents.
 *
 * @param     buf     The buffer to insert the packet into
 *
 * @return    The size of the packet generated
 *****************************************************************************/
int packet_data_err_create(uint8_t* buf)
{
	struct packet* packet = (struct packet*) buf;

	packet->type = ITP_TYPE_DATA_ERR;
	packet->seqnum = 0;
	packet->total = 0;
	packet->size = 0;
	packet->data = 0;
	packet->crc = packet_generate_crc(packet);

	return sizeof(struct packet);
}

/******************************************************************************
 * Creates a CRC checksum on a data buffer of a specified size. This is a
 * private function that should not be called directly.
 *
 * @param     ptr    The data to find the checksum of
 * @param     psize  The size of the data
 *
 * @return    The calculated checksum
 *****************************************************************************/
uint16_t generate_crc(uint8_t *ptr, int psize)
{
	uint16_t crc =0;
	char i;
	while(--psize >= 0){
		crc = crc^ (uint16_t) *ptr++ << 8;
		i = 8;
		do{
			if(crc & 0x8000)
				crc = crc << 1^0x1021;
			else
				crc = crc <<1;
		}while(--i);
	}
	return crc;
}

/******************************************************************************
 * Gets the CRC checksum of a specified packet. The CRC is generated over the
 * packet header as well as data if it exists. The previously set checksum is
 * not included in the CRC calculation.
 *
 * @param     packet  The packet to find the checksum of
 *
 * @return    The calculated checksum
 *****************************************************************************/
uint16_t packet_generate_crc(struct packet* packet)
{
	int size = sizeof(struct packet) - 2;
	if(packet->size > 0)
	{
		size = size - 1 + packet->size;
	}
	return generate_crc( (uint8_t*) &packet->type, size); 
}

/******************************************************************************
 * Checks to see if the packet is correctly structured by first checking the
 * number of bytes and then checking to see if the checksum is correct. The
 * sizing must be compared to the number of bytes recieved.
 *
 * @param     packet        The packet to check
 * @param     recieve_size  The size of the data recieved
 *
 * @return    If the packet is correctly structured
 *****************************************************************************/
int packet_verify_format(struct packet* packet, int recieve_size)
{
	// Check that you got the correct number of bytes
	int packet_size = sizeof(struct packet);

	if(packet->size > 0)
	{
		packet_size = packet_size - 1 + packet->size;
	}

	if(recieve_size != packet_size)
	{
		return 0;
	}

	// Check that the checksum is correct
	if(packet->crc != packet_generate_crc(packet))
	{
		return 0;
	}

	return 1;
}