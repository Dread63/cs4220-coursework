#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define PORT 2222
#define SERVER_IP "127.0.0.1"
#define DATA_FRAME_SIZE 1024
#define FRAME_BYTES (DATA_FRAME_SIZE + 1) // Additional byte for sequence number

int main(void) {

    // Create TCP socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
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

    // Open output file to write recevied data to
    FILE *output_file = fopen("client_files/received.txt", "wb");
    if (!output_file) {
        perror("Failed to open output file");
        close(socket_fd);
        return 1;
    }


    // Initialize frame buffer to receive data
    uint8_t frame[FRAME_BYTES] = {0};
    // Initialize expected sequence value to start at zero   
    uint8_t expected_seq = 0;
    // TODO: Track expected seq byte and send ACK
    while(1) {

        // Read up to a max of FRAME_BYTES
        ssize_t recevied_data = recv(socket_fd, frame, FRAME_BYTES, 0);

        if (recevied_data < 0) {
            
            perror("Recv: error receiving data");
            fclose(output_file);
            close(socket_fd);
            return 1;
        }

        // Server closed the connection
        if (recevied_data == 0) {
            break;
        }

        uint8_t sequence_val = frame[0]; // Pull sequence byte out of received data
        size_t data_length = (recevied_data - 1); // Pull everything other than sequence byte out

        // TODO: If seq != to expected_seq -> re-ACK last good seq and continue
     
        if (sequence_val == expected_seq)
        {
            // Write the data portion of the recevied data
            if (data_length > 0)
            {

                size_t written = fwrite(frame + 1, 1, data_length, output_file);

                if (written != data_length)
                {
                    if (ferror(output_file))
                    {
                        perror("fwrite");
                    }
                    else
                    {
                        fprintf(stderr, "Short write: wrote %zu of %zu bytes\n", written, data_length);
                    }
                    fclose(output_file);
                    close(socket_fd);
                    return 1;
                }

                size_t send_feedback = send(socket_fd, &sequence_val, 1, 0);

                if (send_feedback<= 0)
                {
                    perror("send");
                    return -1;
                }

                // Toggle sequence value
                expected_seq ^= 1;
            }

            else {
                 
            }
        }

        else {


        }
    }

    // TODO: Send ACK sequence byte back to server

    puts("Client: done, closing");
    fclose(output_file);
    close(socket_fd);
    return 0;
}