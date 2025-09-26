#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define PORT 2306
#define SERVER_IP "127.0.0.1"
#define FRAME_SIZE 1024

int main(void) {

    // Initialize the sequence and socket file descriptor values
    int expected_seq = 0, socket_fd;
    char buffer[FRAME_SIZE + 1];

    // Open output file to write recevied data to
    FILE *output_file = fopen("client_files/received.txt", "wb");
    if (!output_file) {
        perror("Failed to open output file");
        close(socket_fd);
        return 1;
    }

    // Create TCP socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        return 1;
    }

    // Configure server address
    struct sockaddr_in address;
    memset(&address, 0, sizeof address);
    address.sin_port = htons(PORT);
    address.sin_family = AF_INET;

    // Convert string address to unit_32 bit value and check for errors
    if (inet_pton(AF_INET, SERVER_IP, &address.sin_addr.s_addr) != 1) {

        perror("inet_pton: invalid server ip");
        close(socket_fd);
        return 1;
    }

    if (connect(socket_fd, (struct sockaddr*)&address, sizeof address) == -1) {
        perror("Failed to connect socket");
        close(socket_fd);
        return 1;
    }
    puts("Client: Connection Successful");


    while(1) {

        int bytes = recv(socket_fd, buffer, FRAME_SIZE + 1, 0);

        // TODO: ERROR HANDLING?
        if (bytes <= 0) {
            
            break;
        }

        int seq = buffer[0];

        if (seq == expected_seq) {

            fwrite(buffer + 1, 1, bytes - 1, output_file);
            send(socket_fd, &expected_seq, sizeof(expected_seq), 0);

            printf("Sent ACK: %d\n", expected_seq);

            // Toggle sequence bit
            expected_seq = 1 - expected_seq;
        }

        else {

            int last_ack = 1 - expected_seq;
            send(socket_fd, &last_ack, sizeof(last_ack), 0);
        }
    }
    
    puts("Client: done, closing");
    fclose(output_file);
    close(socket_fd);
    return 0;
}