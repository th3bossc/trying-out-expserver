#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFF_SIZE 10000

int main() {
    int client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(client_sock_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) != 0) {
        printf("[ERROR]\tFailed to connect to TCP Server\n");
        exit(1);
    }
    printf("[INFO]\tConnected to the TCP Server\n");

    while(1) {
        char* input;
        size_t input_len = 0, read_n;

        read_n = getline(&input, &input_len, stdin);
        send(client_sock_fd, input, input_len, 0);

        read_n = recv(client_sock_fd, input, input_len, 0);

        printf("[SERVER MESSAGE]\t%s\n", input);
    }
}