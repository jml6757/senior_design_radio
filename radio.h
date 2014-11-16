/******************************************************************************
 * File: radio.h
 *
 * Description: Describes radio setup and transmission functionality.
 *****************************************************************************/
#pragma once
#include <stdint.h>

#define MAX_DATA_SIZE 20
#define MAX_BUFFER_SIZE 1500

/******************************************************************************
 * Radio functions
 *****************************************************************************/
int radio_open(const char* device);
void radio_config(int fd, int baud);
int radio_data_send(int fd, uint8_t* data, int size);
int radio_data_receive(int fd, uint8_t* data);
