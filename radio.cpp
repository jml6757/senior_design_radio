#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdlib.h>  /* Used for exiting */
#include <stdio.h>   /* Printing functions */
#include <string.h>  /* Used for memory copies */
#include <poll.h>

#include "packet.h"
#include "radio.h"
#include "log.h"

/******************************************************************************
 * Opens a file descriptor to the radio device. Radio device name is passed
 * in as the argument since these could be different for the beagleboard and
 * the base station.
 *
 * @return    The file descriptor to the radio device
 *****************************************************************************/
int radio_open(const char* device)
{
	int fd;
	fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		log_error("Unable to open %s", device);
		exit(1);
	}
	else
	{
		fcntl(fd, F_SETFL, 1); // Use blocking behavior
	}

	log("Radio Device Opened.\n");
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

	log("Radio Configured.\n");
}

/******************************************************************************
 * Checks to see if data is available to read on the radio device file
 * descriptor. Allows a time to be specified before giving up. Private.
 *
 * @param     fd       The file descriptor to the radio device
 * @param     timeout  The amount of time to wait for data in milliseconds
 *
 * @return    If data is available
 *****************************************************************************/
int radio_data_poll(int fd, int timeout)
{
	// Poll variables
	struct pollfd pfd = {fd, POLLIN | POLLPRI, 0}; // Poll file descriptor
	int n = 0;                                     // Poll return value

	n = poll(&pfd, 1, timeout);
	if(n == -1)
	{
		log_error("Poll failed");
		return 0;
	}
	else if (n == 0)
	{
		log("Warning: No Data. Poll timeout.\n");
		return 0;
	}

	return 1;
}

/******************************************************************************
 * Read radio transmission to a data buffer. Ensures that the packet header is
 * read at minimum. This makes it so that if bytes are sent in separate
 * packets, the whole message will be constructed contiguously. Private.
 *
 * @param     fd      The file descriptor for the radio device
 * @param     data    The data buffer to fill
 *
 * @return    The number of bytes recieved
 *****************************************************************************/
int radio_data_read(int fd, uint8_t* data)
{
	int total = 0;                                 // Total bytes received
	int expected = sizeof(struct packet);          // Bytes expected to be recieved
	struct packet* packet = (struct packet*) data; // Packet Pointer (For ease of use)
	int poll_timeout = 100;                        // Amount of time to wait before giving up

	// Read into the data buffer while while we do not have a complete buffer
	while(total < expected)
	{
		if(radio_data_poll(fd, poll_timeout))
		{
			total += read(fd, data, MAX_BUFFER_SIZE);
		}
		else
		{
			return 0; // Polling failed or timed out
		}
	}

	// If we are getting data, read the data as well
	if(packet->type == ITP_TYPE_DATA_SEND)
	{
		expected += packet->size - 1;
		
		while(total < expected)
		{
			if(radio_data_poll(fd, poll_timeout))
			{
				total += read(fd, data, MAX_BUFFER_SIZE);
			}
			else
			{
				return 0; // Polling failed or timed out
			}
		}
	}

	return total;
}

/******************************************************************************
 * Write the data buffer in chunks.
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
	struct packet* packet = (struct packet*) read_buf;

	// Determine sizing
	int data_size = MAX_DATA_SIZE;
	uint16_t num_packets = size / data_size;
	int n;

	// Send chunks
	log("Starting packet writing...\n");
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
			log("Wrote data chunk - %d of %d (%d Bytes).\n", i+1, num_packets, n);

			// Retry on error
			if(n != packet_size) continue;

			// Attempt to read ACK packet
			n = radio_data_read(fd, read_buf);
			
			// Check that you got the correct number of bytes and correct checksum
			if(packet_verify_format(packet, n) == 1)
			{
				// Check that it is an ACK and not a NACK
				switch(packet->type)
				{
					case ITP_TYPE_DATA_ACK:
						log("ACK Recieved.\n");
						ack_received = 1;
						break;
				
					case ITP_TYPE_DATA_NACK:
						log("NACK Recieved. Retry...\n");
						// TODO: Set current chunk to be sent to the value found in the NACK packet
						continue; // Attempt again

					case ITP_TYPE_DATA_ERR:
						log("Error Recieved. Exiting.\n");
						return -1; // Irrecoverable

					default:
						log("Unknown Packet Type: 0x%X. Exiting.\n", packet->type);
						return -1; // Irrecoverable
				}
			}
			else
			{
				log("Invalid Packet Recieved (%d Bytes).\n", n);
			}
		}
	}
	log("Writing Complete.\n");
	return 0;
}


/******************************************************************************
 * Read radio transmission chunk data to a data buffer. Image data is
 * transmitted in chunks in order to ensure data correctness
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
	struct packet* packet = (struct packet*) read_buf;

	// Read/Write return values
	int n;

	// Counters
	int current = 0;          // The current data chunk to receive
	int final = -1;           // The last data chunk to receive
	uint8_t* data_ptr = data; // Pointer to the end of the data buffer

	// Loop until all packets have been received
	log("Starting packet reading...\n");
	while(current != final)
	{
		// Attempt to recieve buffer
		n = radio_data_read(fd, read_buf);
		
		// Check that you got the correct number of bytes and correct checksum
		if(packet_verify_format(packet, n) == 1)
		{
			// Set the final packet number if it has not already been
			if(final == -1)
			{
				final = packet->total;
			}

			log("Read data chunk - %d of %d (%d Bytes).\n", current+1, final, n);

			// If the sequence number is too large, inform that you did not receive the current packet
			if(packet->seqnum > current)
			{
				log("Incorrect Sequence Number. Writing NACK.\n");
				int packet_size = packet_data_nack_create(write_buf, current, final);
				n = write(fd, write_buf, packet_size);
				continue;
			}

			log("Writing ACK.\n");
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
			log("Invalid Packet Recieved (%d Bytes). Writing NACK...\n", n);
			// When packet is ill-formatted, send NACK
			int packet_size = packet_data_nack_create(write_buf, current, final);
			n = write(fd, write_buf, packet_size);
		}
	}
	
	log("Reading Complete.\n");
	return data_ptr - data;
}