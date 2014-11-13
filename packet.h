/******************************************************************************
 * File: packet.h
 *
 * Description: Describes the image transfer protocol (ITP) packet structure
 *              packet creation functions, and packet helper functions
 *****************************************************************************/
#pragma once
#include <stdint.h>

/******************************************************************************
 * Image transfer protocol packet structure
 *****************************************************************************/
struct packet
{
	uint16_t  crc;     // The packet checksum
	uint8_t   type;    // The type of packet (Data, ACK, NACK)
	uint16_t  seqnum;  // The packet sequence number out of total packets
	uint16_t  total;   // Total packets
	uint16_t  size;    // Total size of the data
	uint8_t   data;    // Start of the data
};

/******************************************************************************
 * Packet types
 *****************************************************************************/
#define ITP_TYPE_DATA_SEND 0x01 // Data is being sent
#define ITP_TYPE_DATA_ACK  0x02 // Data was recieved successfully
#define ITP_TYPE_DATA_NACK 0x03 // Data was recieved unsuccessfully
#define ITP_TYPE_DATA_ERR  0x04 // Data was recieved but there was an error

/******************************************************************************
 * Packet creation methods
 *****************************************************************************/
int packet_data_send_create(uint8_t* buf, uint8_t* data, uint16_t size, uint16_t seqnum, uint16_t total);
int packet_data_ack_create(uint8_t* buf, uint16_t seqnum, uint16_t total);
int packet_data_nack_create(uint8_t* buf, uint16_t seqnum, uint16_t total);
int packet_data_err_create(uint8_t* buf);

/******************************************************************************
 * Helper functions
 *****************************************************************************/
uint16_t packet_generate_crc(struct packet* packet);
int packet_verify_format(struct packet* packet, int recieve_size);