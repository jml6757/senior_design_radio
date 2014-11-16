/******************************************************************************
 * File: sim.h
 *
 * Description: Defines file descriptor creation functions for the test
 *              simulation
 *****************************************************************************/

#pragma once

#define SIM_PORT 12345

int sim_tcp_server_socket();
int sim_tcp_client_socket(const char* ip);
