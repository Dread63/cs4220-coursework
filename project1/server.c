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

#define PORT "2222"
#define MAX_BACKLOG 5
#define FRAME_SIZE 1024

#define STDIN 0

int send_file_data(int socket_fd, const char *filepath);

int main(void)
{

    // Used to store current and new socket file descriptors
    int socket_fd, new_fd;

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

    // send_file_data returns 0 only when file is successfully read
    if (send_file_data(new_fd, "inputfile.txt") != 0)
    {
        perror("send_file_data failed\n");
        close(new_fd);
        close(socket_fd);
        return 1;
    }

    close(new_fd);
    close(socket_fd);
    puts("Server: done, closing connection");
    return 0;
}

// Send file data to client in blocks over the open socket connection
int send_file_data(int socket_fd, const char *filepath)
{

    FILE *data_file = fopen(filepath, "rb");

    if (data_file == NULL)
    {

        perror("datafile");
        return -1;
    }

    // Initialize seq, full frame buffer, and reading buffer
    uint8_t current_sequence = 0;
    uint8_t current_frame[FRAME_SIZE + 1];
    char buffer[FRAME_SIZE] = {0};

    // Loop to read through file and send data for each 1024 block read
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, FRAME_SIZE, data_file)) > 0)
    {

        current_frame[0] = current_sequence;

        // Copy all data from file after the first sequence bit
        memcpy(&current_frame[1], buffer, bytes_read);

        size_t to_send = 1 + bytes_read;

        // Loop to handle sending/receiving of ACK
        while(1)
        {
            size_t sent = 0;
            while (sent < to_send)
            {
                ssize_t send_feedback = send(socket_fd, current_frame + sent, to_send - sent, 0);
                
                // Check if data was sent successfully
                if (send_feedback<= 0)
                {
                    perror("send");
                    fclose(data_file);
                    return -1;
                }

                sent += send_feedback;
            }

            // Wait for 1-byte ack using timout
            struct timeval tv = {.tv_sec = 2, .tv_usec = 500000};
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(socket_fd, &rfds);
            int select_feedback = select(socket_fd + 1, &rfds, NULL, NULL, &tv);
            
            // Check if socket is readable
            if (select_feedback < 0)
            {
                perror("select");
                fclose(data_file);
                return -1;
            }

            // Check for timeout and re-send frame if needed
            if (select_feedback == 0)
            {
                puts("Timed out: resending frame");
                continue;
            }

            // Something can be read, check if ack is correct
            if (FD_ISSET(socket_fd, &rfds))
            {
                uint8_t ack;
                ssize_t received_feedback = recv(socket_fd, &ack, 1, 0);
    
                if (received_feedback <= 0)
                {
                    perror("recv ack");
                    fclose(data_file);
                    return -1;
                }

                printf("Server: ACK received %u\n", ack);

                // If ack matches client feedback, toggle current sequence
                if (ack == current_sequence)
                {   
                    printf("Server: sent seq=%u, bytes=%zu\n", current_sequence, bytes_read);
                    current_sequence ^= 1;
                    break;                 
                }

                // Re-send ack if client feedback doesn't match
                else
                {
                    puts("Wrong ACK: resending frame");
                }
            }
        }

        bzero(buffer, FRAME_SIZE);
    }

    fclose(data_file);
    return 0;
}