#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFFER 1024
#define PORT 8080
#define SERVER_IP "127.0.0.1"

int main(void) {

    // Init OpenSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // Create the SSL context object
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        fprintf(stderr, "SSL_CTX_new() failed.\n");
        return 1;
    }

    // Configure the ssl context object to use our self-signed certificate in the root directly
    if (!SSL_CTX_use_certificate_file(ctx, "cert.pem" , SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM)) {
        fprintf(stderr, "SSL_CTX_use_certificate_file() failed.\n");
        ERR_print_errors_fp(stderr);

        return 1;
    }

    int socket_fd = 0;
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

    // Loop to send and receive data with client
    while (1) {

        // Create new SSL object to wrap our accepted TCP connection in
        SSL *ssl = SSL_new(ctx);
        if (!ssl) {
            fprintf(stderr, "SSL_new() failed.\n");
            return 1;
        }

        

        int bytes = SSL_read(socket_fd, buffer, BUFFER);

    }
}