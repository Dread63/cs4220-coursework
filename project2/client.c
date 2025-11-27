#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER 1024
#define PORT 8080
#define SERVER_IP "127.0.0.1"

int main(void) {

    int socket_fd;
    char buffer[BUFFER];

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
}
