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

#define PORT "2222"
#define MAX_BACKLOG 5
#define FRAME_SIZE 1024

int send_file_data(int socket_fd, const char *filepath);


int main(void) {

    // Used to store current and new socket file descriptors
    int socket_fd, new_fd;

    // Used to track error status
    int err_status;

    struct addrinfo hints;
    struct addrinfo *servinfo, *p;

    socklen_t sin_size;

    // Address of client connecting to server
    struct sockaddr_storage client_addr;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
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

        if (socket_fd == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("bind");
            continue;
        }

        bind_success = true;
    }

    // Free up memory dedicated to the server info structure
    freeaddrinfo(servinfo);

    if (listen(socket_fd, MAX_BACKLOG) == -1) {

        perror("Listen error: Too many connections in queue");
        exit(1);
    }
    printf("Listening....\n");

    // Accept connection from client
    sin_size = sizeof client_addr;
    new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        close(socket_fd);
        return 1;
    }
    printf("Connection accepted\n");

    // Reading file
    send_file_data(new_fd, "inputfile.txt");

    return 0;
}

int send_file_data(int socket_fd, const char *filepath) {

    FILE *data_file = fopen(filepath, "rb");

    uint8_t current_sequence = 0;
    uint8_t current_frame[FRAME_SIZE + 1];

    if (data_file == NULL) {

        perror("datafile");
        return -1;
    }

    // Buffer used to read inputfile.txt
    char buffer[FRAME_SIZE] = {0};

    size_t bytes_read;
    while((bytes_read = fread(buffer, 1, FRAME_SIZE,  data_file)) > 0) {

        current_frame[0] = current_sequence;

        // Copy all data from file after the first sequence bit
        memcpy(&current_frame[1], buffer, bytes_read);

        size_t to_send = 1 + bytes_read;
        if(send(socket_fd, current_frame, to_send, 0) == -1) {
            perror("send");
            exit(1);
        }

        bzero(buffer, FRAME_SIZE);
        //uint8_t ack;
        //recv(clientsocket, ack, sizeof(ack), 0);

        // Continue re-transmitting current frame until ack matches
        //while (ack != current_frame) {

          //  write(socket, current_frame, FRAME_SIZE + 1);
            //recv(clientsocket, ack, sizeof(ack), 0);
        //}
    }
}