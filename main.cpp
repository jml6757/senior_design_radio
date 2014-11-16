#include <termios.h> /* POSIX terminal control definitions */
#include <stdio.h>   /* Printing functions */

#include "packet.h"
#include "radio.h"
#include "log.h"
#include "sim.h"

int beagleboard_main()
{
	log("Beagleboard Started.\n");

	int fd = 0;

	// Get the transmission device file descriptor
	fd = sim_tcp_server_socket();
	// fd = radio_open();
	// radio_config(fd, B57600); // 8N1 @ 57600

	// Allocate data buffer
	uint8_t data[MAX_BUFFER_SIZE] = {0};

	// Retrieve data
	radio_data_receive(fd, data);

	return 0;
}

int base_main()
{
	log("Base Station Started.\n");
	int fd = 0;

	// Get the transmission device file descriptor
	fd = sim_tcp_client_socket("10.0.0.1");
	// fd = radio_open();
	// radio_config(fd, B115200); // 8N1 @ 115200

	// Create data buffer
	uint8_t data[20] = {0};

	// Fill buffer
	for(int i=0; i<20; i++)
	{
		data[i] = i;
	}

	// Transmit data
	radio_data_send(fd, data, 20);

	return 0;
}

int main()
{
	// Run code for specified device
	return base_main();
}