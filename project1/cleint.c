#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 2222
#define SERVER_IP "127.0.0.1"
#define DATA_FRAME_SIZE 1024

int main() {

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int file_data;

    struct sockaddr_in address;
    memset(&address, 0, sizeof address);
    address.sin_port = htons(PORT);
    address.sin_family = AF_INET;

    // Convert string address to unit_32 bit value
    inet_pton(AF_INET, SERVER_IP, &address.sin_addr.s_addr);

    int result = connect(socket_fd, (struct sockaddr*)&address, sizeof address);

    if (result == 0) {

        printf("Connection succesful\n");
    }

    else {
        printf("Error Connecting");
    }

    FILE *output_file = fopen("client_files/received.txt", "wb");
    char buffer[DATA_FRAME_SIZE + 1] = {0};

    while(1) {

        file_data = recv(socket_fd, buffer, DATA_FRAME_SIZE + 1, 0);

        if (file_data <= 0) {
            break;
            return -1;
        }

        size_t written = fwrite(buffer + 1, 1, DATA_FRAME_SIZE, output_file);
        if (written != DATA_FRAME_SIZE) {
            perror("fwrite");
            return -1;
        }

        puts("Done writing output file");
        break;
    }

    return 0;
}
