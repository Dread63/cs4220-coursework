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

    // Configure the ssl context object to use our self-signed certificate in the root directly
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

        // Create new SSL object to wrap our accepted TCP connection in
        SSL *ssl = SSL_new(ctx);
        if (!ssl) {
            fprintf(stderr, "SSL_new() failed.\n");
            return 1;
        }

        // Accept connection from client, casting client_addr address as a sockaddr pointer
        socklen_t sin_size = sizeof(client_addr);
        new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &sin_size);

        // Check for error on accepting connection from client
        if (new_fd == -1) {

            fprintf(stderr, "Failed to accept connection from client\n");
            continue;
        }
        puts("Server: connection accepted successfully\n");

        // Wrap the TCP connection in new_fd using SSL
        SSL_set_fd(ssl, new_fd);
        if (SSL_accept(ssl) <= 0) {
            fprintf(stderr, "SSL_accept() failed.\n");
            ERR_print_errors_fp(stderr);
            continue;
        }

        ssize_t bytesRead;

        // Read the data coming from client using SSL_read for secure communication
        if ((bytesRead = SSL_read(ssl, buffer, BUFFER - 1)) <= 0) {

            fprintf(stderr, "SSL_read() failed.\n");
        }

        // After reading HTTP data from client, send a response
        else {

            buffer[bytesRead] = '\0'; // Null terminate the received string
            puts(buffer); // Print received HTTP data

            // Send HTTP response
            const char *response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 44\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<html><body><h1>Hello, World!</h1></body></html>";

            if (SSL_write(ssl, response, strlen(response)) <= 0) {
                fprintf(stderr, "SSL_write() failed.\n");
                ERR_print_errors_fp(stderr);
            }
        }

        // Free SSL and socket resources after connection closes
        SSL_shutdown(ssl);
        close(new_fd);
        SSL_free(ssl);
    }

    // These cleanup operations should be done even though there isn't currently
    // an operation closing the server connection (manual intervention such as CRL + C)
    // should end the server since we expect it to run indefinitely.
    close(socket_fd);
    SSL_CTX_free(ctx);
    return 0;
}