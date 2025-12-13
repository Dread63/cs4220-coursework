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
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        fprintf(stderr, "SSL_CTX_new() failed.\n");
        return 1;
    }

    // Set minimum TLS protocol to TLS 1.2
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    // Disable all other insecure protocols
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

    // Configure the ssl context object to use our self-signed certificate in the root directly
    if (!SSL_CTX_use_certificate_file(ctx, "client.crt" , SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx, "client.key", SSL_FILETYPE_PEM)) {
        fprintf(stderr, "SSL_CTX_use_certificate_file() failed.\n");
        ERR_print_errors_fp(stderr);

        return 1;
    }

    // Load server certificate to verify (for mTLS)
    if (!SSL_CTX_load_verify_locations(ctx, "server.crt", NULL)) {
        fprintf(stderr, "Failed to load server CA certificate\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 1;
    }

    // Enable server certificate verification
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    // Set verification depth
    SSL_CTX_set_verify_depth(ctx, 1);

    // Prevents incomplete read/write during negotiation
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

    // Create TCP socket
    int socket_fd = 0;
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
    if (inet_pton(AF_INET, SERVER_IP, &address.sin_addr) != 1) {

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

    // Now that a connection is made, we need to wrap the socket in SSL
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        perror("Failed to create SSL object");
        close(socket_fd);
        return 1;
    }

    // Wrap the socket_fd using SSL
    SSL_set_fd(ssl, socket_fd);
    // Connect to the server over ssl (any return value other than 1 indicates an error)
    if (SSL_connect(ssl) <= 0) {
        fprintf(stderr, "SSL_connect() failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }

    puts("Client: SSL handshake successful");

    // Send HTTP GET request to server
    const char *request =
        "Request for greeting!";

    if (SSL_write(ssl, request, strlen(request)) <= 0) {
        fprintf(stderr, "SSL_write() failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }

    puts("Client: Sent HTTP request");

    // Receive response from server
    char buffer[BUFFER];
    int bytes = SSL_read(ssl, buffer, BUFFER - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0'; // Null terminate received string
        printf("Client: Received HTTP response: %s\n", buffer);
    }
    else if (bytes == 0) {
        puts("Client: Server closed connection");
    }
    else {
        fprintf(stderr, "SSL_read() failed.\n");
        ERR_print_errors_fp(stderr);
    }

    // Free up memory
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(socket_fd);
    SSL_CTX_free(ctx);

    puts("Client: Connection successfully closed");
    return 0;
}