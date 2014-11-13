#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdlib.h>  /* Used for exiting */
#include <stdio.h>   /* Printing functions */
#include <string.h>  /* Used for memory copies */

#include "packet.h"
#include "radio.h"

/******************************************************************************
 * Opens a file descriptor to the radio device. The device name is hardcoded
 * since it will always be the same.
 *
 * @return    The file descriptor to the radio device
 *****************************************************************************/
int radio_open(void)
{
	int fd;
	fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		perror("Unable to open /dev/ttyO0 - ");
		exit(1);
	}
	else
	{
		fcntl(fd, F_SETFL, 1); // Use blocking behavior
	}
	return (fd);

}

/******************************************************************************
 * Initializes the serial options associated with an open file descriptor. This
 * ensures that the data will be formatted using raw data 8N1.
 *
 * @param     fd      The file descriptor to the radio device
 * @param     baud    The baud rate to set the serial output to
 *****************************************************************************/
void radio_config(int fd, int baud)
{
	struct termios options;

	// Get current attributes
	tcgetattr(fd, &options);

	// Change intput/output baud rate
	cfsetispeed(&options, baud);
	cfsetospeed(&options, baud);

	// PARENB | Disable parity
	// CSTOPB | Use 1 stop bit
	// CSIZE  | Reset data bit size
	options.c_cflag &= ~( PARENB | CSTOPB | CSIZE);

	// CLOCAL | Do not change own of port
	// CREAD  | Enable receiver
	// CS8    | Set 8 data bits
	options.c_cflag |= (CLOCAL | CREAD | CS8);

	// ICANON | Disable canonical output (else raw)
	// ECHO   | Disable echoing of input characters
	// ECHOE  | Disable echoing of erase characters
	// ISIG   | Disable signals (i.e. SIGINTR)
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	// Set the attributes
	tcsetattr(fd, TCSANOW, &options);
}

/******************************************************************************
 * Write the data buffer.
 *
 * @param     fd      The file descriptor to the radio device
 * @param     data    The data to send
 * @param     size    The size of the data to send
 *
 * @return    If transfer was successful (0 = success, -1 = failure)
 *****************************************************************************/
int radio_data_send(int fd, uint8_t* data, int size)
{
	// Allocate packet buffers
	uint8_t write_buf[MAX_BUFFER_SIZE] = {0};
	uint8_t read_buf[MAX_BUFFER_SIZE] = {0};
	struct packet* packet;

	// Determine sizing
	int data_size = MAX_DATA_SIZE;
	uint16_t num_packets = size / data_size;
	int n;

	// Send chunks
	for(int i = 0; i < num_packets; ++i)
	{
		// Ensure correct data size when at the end of data
		if(i+1 == num_packets)
		{
			data_size = (size - (MAX_DATA_SIZE*i));
		}
		
		// Create packet
		int packet_size = packet_data_send_create(write_buf, data + (MAX_DATA_SIZE* i), data_size, i, num_packets);

		// Send the packet and wait for an ACK
		int ack_received = 0;
		while(ack_received == 0)
		{
			// Send the data chunk
			n = write(fd, write_buf, packet_size);
			
			// Retry on error
			if(n != packet_size) continue;

			// Attempt to read ACK packet
			n = read(fd, read_buf, MAX_BUFFER_SIZE);
			
			// Check that you got the correct number of bytes and correct checksum
			packet = (struct packet*) read_buf;
			if(packet_verify_format(packet, n) == 1)
			{
				// Check that it is an ACK and not a NACK
				switch(packet->type)
				{
					case ITP_TYPE_DATA_ACK:
						ack_received = 1;
						break;
				
					case ITP_TYPE_DATA_NACK:
						continue; // Attempt again

					case ITP_TYPE_DATA_ERR:
						return -1; // Irrecoverable
				}
			}
		}
	}
	return 0;
}


/******************************************************************************
 * Read radio transmission to a data buffer.
 *
 * @param     fd      The file descriptor to the radio device
 * @param     data    The data buffer to fill
 *
 * @return    The number of bytes recieved
 *****************************************************************************/
int radio_data_receive(int fd, uint8_t* data)
{
	// Allocate packet buffers
	uint8_t write_buf[MAX_BUFFER_SIZE] = {0};
	uint8_t read_buf[MAX_BUFFER_SIZE] = {0};
	struct packet* packet;

	// Read/Write return values
	int n;

	// Counters
	int current = 0; // The current data chunk to receive
	int final = -1;  // The last data chunk to receive
	uint8_t* data_ptr = data;

	// Loop until all packets have been received
	while(current < final)
	{
		// Attempt to recieve buffer
		n = read(fd, read_buf, MAX_BUFFER_SIZE);

		// Check that you got the correct number of bytes and correct checksum
		packet = (struct packet*) read_buf;
		if(packet_verify_format(packet, n) == 1)
		{
			// Set the final packet number if it has not already been
			if(final == -1)
			{
				final = packet->total;
			}

			// If the sequence number is too large, inform that you did not receive the current packet
			if(packet->seqnum > current)
			{
				int packet_size = packet_data_nack_create(write_buf, current, final);
				n = write(fd, write_buf, packet_size);
				continue;
			}

			// Sent acknowledgement of current packet (This must send even if a redundant packet was sent)
			int packet_size = packet_data_ack_create(write_buf, packet->seqnum, final);
			n = write(fd, write_buf, packet_size);

			// Only add data to buffer if it is newer
			if(packet->seqnum == current)
			{
				// Increment the current counter
				current++;

				// Append data buffer with the new data
				memcpy(data_ptr, &packet->data, packet->size);

				// Move the data pointer
				data_ptr += packet->size;
			}

		}
		else
		{
			// When packet is ill-formatted, send NACK
			int packet_size = packet_data_nack_create(write_buf, current, final);
			n = write(fd, write_buf, packet_size);
		}
	}
	
	return data_ptr - data;
}