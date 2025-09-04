#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 2222
#define SERVER_IP "142.250.69.238"
#define DATA_FRAME_SIZE 1024

int main() {

    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_port = htons(PORT);
    address.sin_family = AF_INET;

    // Convert string address to unit_32 bit value
    inet_pton(AF_INET, SERVER_IP, &address.sin_addr.s_addr);

    int result = connect(socketFD, (struct sockaddr*)&address, sizeof address);

    if (result == 0) {

        printf("Connection succesful\n");
    }

    FILE *outputFiels = fopen("client_files/received.txt", "wb");

    

    return 0;
}