#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sim.h"
#include "log.h"

/******************************************************************************
 * Creates a TCP listen socket for the TCP server. For the purposes of the
 * simulation, this is a private function.
 *
 * @return    The file descriptor to the TCP listen socket 
 *****************************************************************************/
int sim_tcp_listen_socket()
{
    int sockfd;
    struct sockaddr_in serv_addr;

    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        log_error("Opening TCP listen socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SIM_PORT);
 
    /* Bind the host address */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
         log_error("Binding TCP listen socket");
         exit(1);
    }


    return sockfd;
}

/******************************************************************************
 * Creates the server TCP socket for simulation.
 *
 * @return    The file descriptor to the simulation radio device (server)
 *****************************************************************************/
int sim_tcp_server_socket()
{
    int listen_sock;
    int sockfd;

    /* Create listen socket */
    listen_sock = sim_tcp_listen_socket();

    /* Listen for connection request*/
    listen(listen_sock, 5);

    /* Accept connection from the client */
    sockfd = accept(listen_sock, (struct sockaddr *) NULL, NULL);
    if (sockfd < 0) 
    {
        log_error("TCP Accept");
        exit(1);
    }

    /* Close the listen port */
    close(listen_sock);

    log("TCP Socket Created.\n");
    return sockfd;
}

/******************************************************************************
 * Creates the client TCP socket for simulation.
 *
 * @param     ip    The IP address of the TCP server
 *
 * @return    The file descriptor to the simulation radio device (client)
 *****************************************************************************/
int sim_tcp_client_socket(const char* ip)
{
    int sockfd;
    struct sockaddr_in serv_addr; 
    
    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        log_error("Binding TCP socket");
        exit(1);
    }

    /* Create destination address */
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SIM_PORT); 
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
    {
        log_error("Address conversion");
        exit(1);
    } 

    /* Connect to the tcp server */
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        log_error("TCP Connect");
        exit(1);
    } 

    log("TCP Socket Created.\n");
    return sockfd;
}