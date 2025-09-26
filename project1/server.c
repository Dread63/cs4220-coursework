#define _POSIX_C_SOURCE 200112L
#include <stdint.h>           
#include <sys/select.h>         
#include <sys/time.h>          
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

#define PORT "2306"
#define MAX_BACKLOG 5
#define FRAME_SIZE 1024

int main(void)
{

    // Used to store current and new socket file descriptors
    int socket_fd, new_fd;
    
    FILE *input_file = fopen("input_file.txt", "rb");
    char buffer[FRAME_SIZE + 1];
    int seq = 0;

    // Used to track error status
    int err_status;

    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((err_status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "gai error: %s\n", gai_strerror(err_status));
        return 1;
    }

    // Condition to break out of the loop when a socket is successfully bound
    bool bind_success = false;
    for (p = servinfo; p != NULL && !bind_success; p = p->ai_next)
    {

        // Socket for current address
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        // Check if socket was initialized successfully
        if (socket_fd == -1)
        {
            perror("socket");
            continue;
        }

        // Used to make sure we don't get "address already in use when running code again"
        int yes = 1;
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        // Try to bind and print error if can't
        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(socket_fd);
            perror("bind");
            continue;
        }

        bind_success = true;
    }

    // Free up memory dedicated to the server info structure
    freeaddrinfo(servinfo);

    // Check if binding was successful
    if (!bind_success)
    {
        perror("Server failed to bind");
        return 1;
    }

    // Open socket for listening
    if (listen(socket_fd, MAX_BACKLOG) == -1)
    {

        perror("Listen error: Too many connections in queue");
        close(socket_fd);
        return 1;
    }
    printf("Listening....\n");

    // Address of client connecting to server
    struct sockaddr_storage client_addr;

    // Accept connection from client
    socklen_t sin_size = sizeof client_addr;
    new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &sin_size);

    // Check if there was an error accepting the connection
    if (new_fd == -1)
    {
        perror("accept");
        close(socket_fd);
        return 1;
    }

    puts("Server: connection accepted\n");

    // While there are still lines in the file to read
    int bytes = 0;
    do {

        bytes = fread(buffer + 1, 1, FRAME_SIZE, input_file);

        // Set index 0 equal to the current sequence bit
        buffer[0] = seq;

        // Loop to keep retransmitting frame based on ACK
        while(1) {

            int sent = send(new_fd, buffer, bytes + 1, 0);

            // Wait for 1-byte ack using timout
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(new_fd, &rfds);

            // Re-initialize timval struct on every loop since select wipes it
            struct timeval tv = {.tv_sec = 2, .tv_usec = 500000};

            int select_feedback = select(new_fd + 1, &rfds, NULL, NULL, &tv);

            // If bits were read into the read file descriptor & socket_fd is a member of the rfds set
            if (select_feedback > 0 && FD_ISSET(new_fd, &rfds)) {

                int ack = 0;
                int ack_recevied = recv(new_fd, &ack, sizeof(ack), 0);

                if (ack_recevied > 0 && ack == seq) {
                    printf("Expected ack: %d, received ack: %d\n", seq, ack);

                    // Toggle sequence value
                    seq = 1 - seq;
                    break;
                }

                else {

                    printf("Wrong ack received: %d\n", ack);
                }
            }

            puts("Timed out, re-sending frame\n");
        }  
    } while (bytes != 0);

    puts("Done writing file to socket");

    fclose(input_file);
    close(socket_fd);
    close(new_fd);
    return 0;
}