#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#define BUFFER 1024
#define PORT "8080"
#define MAX_BACKLOG 10

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

    if (!SSL_CTX_use_certificate_file(ctx, "cert.pem" , SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM)) {
        fprintf(stderr, "SSL_CTX_use_certificate_file() failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
}

    int socket_fd = 0, new_fd = 0;

    char buffer[BUFFER];

    int status; // Integer used to check for errors
    struct addrinfo hints; // Structure for configuration
    struct addrinfo *servinfo; // Pointer to receive linked list of addrinfo structures
    struct addrinfo *p; // Pointer used to loop through linked list of addrinfo structs

    memset(&hints, 0, sizeof hints); // Zero-out structure memory
    hints.ai_family = AF_INET; // Choose IPV4 
    hints.ai_socktype = SOCK_STREAM; // Choose TCP stream socket
    hints.ai_flags = AI_PASSIVE; // Use localhost IP address for binding

    // Prepare server info for connection and check for errors
    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {

        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        exit(1);
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {

        // Create socket and check for errors
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {

            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            continue; // Continue to trying next address
        }

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            fprintf(stderr, "Failed to bind socket\n");
            continue;
        }

        break;
    }
    
    freeaddrinfo(servinfo); // Free up linked list of addresses

    // Check if there was a failure to bind
    if (p == NULL) {
       fprintf(stderr, "Failed to bind socket\n");
        exit(1);
    }

    // Listen for connection, else print error
    if (listen(socket_fd, MAX_BACKLOG) == -1) {
        fprintf(stderr, "Failed to start listener\n");
        exit(1);
    }

    puts("Server: Listening...");

    // Everything setup properly on server, accept connection from client
    struct sockaddr_storage client_addr;

    while(1) {

        // Accept connection from client, casting client_addr address as a sockaddr pointer
        socklen_t sin_size = sizeof(client_addr);
        new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &sin_size);

        // Check for error on accepting connection from client
        if (new_fd == -1) {

            fprintf(stderr, "Failed to accept connection from client\n");
            continue;
        }

        else {

            puts("Server: connection accepted successfully\n");
        }

        ssize_t bytesRead;

        if ((bytesRead = read(new_fd, buffer, BUFFER)) == -1) {

            fprintf(stderr, "Error reading HTTP request");
            exit(1);
        }

        // Null terminate the received string
        if (bytesRead >= 0) {
            buffer[bytesRead] = '\0';
            puts(buffer);

            // Send HTTP response
            const char *response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 44\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<html><body><h1>Hello, World!</h1></body></html>";

            send(new_fd, response, strlen(response), 0);
        }

        else {
            
            fprintf(stderr, "Error reading buffer\n");
        }

        close(new_fd);
        break;
    }

    close(socket_fd);
    return 0;
}