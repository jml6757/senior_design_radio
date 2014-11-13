#include <termios.h> /* POSIX terminal control definitions */
#include <stdio.h>   /* Printing functions */

#include "packet.h"
#include "radio.h"

int main()
{
	// Initialize radio
	int fd = radio_open();
	radio_config(fd, B57600); // 8N1 @ 57600

	printf("Initialized Radio: %d\n", fd);

	// Create data buffer
	uint8_t data[20] = {0};

	// Fill buffer
	for(int i=0; i<20; i++)
	{
		data[i] = i;
	}

	// Transmit data
	radio_data_send(fd, data, 20);
}