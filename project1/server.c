#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <stdbool.h>

#define PORT "2222"
#define MAX_BACKLOG 5

int main(void) {

    // Used to store current and new socket file descriptors
    int socket_fd, new_fd;

    // Used to track error status
    int err_status;

    struct addrinfo hints;
    struct addrinfo *servinfo, *p;

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

    if (listen(socket_fd, MAX_BACKLOG) != 0) {

        perror("Listen error: Too many connections in queue");
        exit(1);
    }



    return 0;
}