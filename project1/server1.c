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
#include <string.h>

#define DATA_SIZE 1024      // payload bytes per frame (after seq byte)
#define FRAME_SIZE 1025     // 1 seq byte + 1024 payload
#define PORT "2222"
#define MAX_BACKLOG 10

int send_all(int sockfd, const void *buf, size_t len) {
    
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sockfd, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1; // should not happen unless connection closed
        total += (size_t)n;
    }
    return 0;
}

int send_file_data(int socket_fd, const char *filepath, uint8_t seq)
{
    FILE *data_file = fopen(filepath, "rb");
    if (!data_file) {
        perror("fopen");
        return -1;
    }

    uint8_t frame[FRAME_SIZE];  // [0] = seq, [1..] = data

    for (;;) {
        frame[0] = seq; // sequence/ACK byte at start of every frame

        // read up to DATA_SIZE bytes into frame starting at index 1
        size_t bytes_read = fread(&frame[1], 1, DATA_SIZE, data_file);

        if (bytes_read == 0) {
            if (feof(data_file)) {
                // EOF reached cleanly
                break;
            }
            // read error
            perror("fread");
            fclose(data_file);
            return -1;
        }

        if (bytes_read < DATA_SIZE) {
            memset(&frame[1 + bytes_read], 0, DATA_SIZE - bytes_read);
        }

        // send exactly (1 + bytes_read) bytes
        if (send_all(socket_fd, frame, 1 + bytes_read) == -1) {
            perror("send");
            fclose(data_file);
            return -1;
        }

        // (Optional) toggle seq for Stop-and-Wait 0/1 alternating:
        seq ^= 0x01;
    }

    fclose(data_file);
    return 0;
}

int send_all(int sockfd, const void *buf, size_t len);
int send_file_data(int socket_fd, const char *filepath, uint8_t seq);

int main(void) {


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

    uint8_t seq = 0;

    // send_file_data returns 0 only when file is successfully read
    if (send_file_data(new_fd, "inputfile.txt", seq) != 0) {
        // send_file_data already printed a perror; add context if you like
        fprintf(stderr, "send_file_data failed\n");
        close(new_fd);
        close(socket_fd);
        return 1;
    }
}