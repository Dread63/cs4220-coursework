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

int main(void) {

    // Used to store current and new socket file descriptors
    int socket_fd, new_fd;

    // Used to track error status
    int err_status;
    
    // OUTLIER
    socklen_t sin_size;
  

    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;

    if ((err_status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(err_status));
        return 1;
    }

    // Condition to break out of the loop when a socket is successfully bound
    bool bind_success = false;

    for (p = servinfo; p != NULL && !bind_success; p = p->ai_next) {

        // Socket for current address
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        // Check if socket was initialized successfully
        if (socket_fd == -1) {
            perror("socket");
            continue;
        }

        // Try to bind and print error if can't
        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("bind");
            continue;
        }

        bind_success = true;
    }

    // Free up memory dedicated to the server info structure
    freeaddrinfo(servinfo);

    // Check if binding was successful
    if (!bind_success) {
        perror("Server failed to bind");
        return 1;
    }

    // Open socket for listening
    if (listen(socket_fd, MAX_BACKLOG) == -1) {

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
    if (new_fd == -1) {
        perror("accept");
        close(socket_fd);
        return 1;
    }

    puts("Server: connection accepted\n");

    if(send_file_data(new_fd, "inputfile.txt") != 0) {
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

int send_file_data(int socket_fd, const char *filepath) {

    FILE *data_file = fopen(filepath, "rb");

    if (data_file == NULL) {

        perror("datafile");
        return -1;
    }

    uint8_t current_sequence = 0;
    uint8_t current_frame[FRAME_SIZE + 1];
    char buffer[FRAME_SIZE] = {0};

    size_t bytes_read;
    while((bytes_read = fread(buffer, 1, FRAME_SIZE,  data_file)) > 0) {

        current_frame[0] = current_sequence;

        // Copy all data from file after the first sequence bit
        memcpy(&current_frame[1], buffer, bytes_read);

        size_t to_send = 1 + bytes_read;
        if(send(socket_fd, current_frame, to_send, 0) == -1) {
            perror("send");
            fclose(data_file);
            return -1;
        }

        // TODO: start timer, wait for ACK (sequence). On timeout/incorrect ACK, retransmit same frame
        struct timeval tv;
        fd_set readfds;

        tv.tv_sec = 2;
        tv.tv_usec = 500000;

        FD_ZERO(&readfds);
        FD_SET(socket_fd, &readfds);

        // don't care about writefds and exceptfds:
        int select_feedback = select(socket_fd + 1, &readfds, NULL, NULL, &tv);

        if (select_feedback == -1) {
            perror("Select error");
            close(socket_fd);
            return -1;
        }

        else if (select_feedback == 0) {
            puts("Timed out: Resending Frame");
            
            // Continue to re-send current frame
        }

        else {

            if (FD_ISSET(socket_fd, &readfds)) {

                uint8_t ack_buffer[2];
                ssize_t recevied_data = recv(socket_fd, ack_buffer, sizeof ack_buffer, 0);

                if (recevied_data > 0) {

                    printf("Server: ACK Received %d\n", ack_buffer[0]);

                    if(ack_buffer[0] != current_sequence) {

                        // RESEND CURRENT FRAME
                    }

                    else {
                        continue;
                    }
                }

                else {
                    
                }
            }
        }


        // printf("Server: sent seq=%u, bytes=%zu\n", seq, bytes_read);

        // TODO: toggle sequence byte after valid ACK
        bzero(buffer, FRAME_SIZE);
    }

    fclose(data_file);
    return 0;
}