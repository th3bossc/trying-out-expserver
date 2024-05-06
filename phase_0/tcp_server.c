#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPT_BACKLOG 5


void strrev(char* input) {
    for (int start = 0, end = strlen(input)-2; start < end; start++, end--) {
        char temp = input[start];
        input[start] = input[end];
        input[end] = temp;
    }
}   



int main() {
    int listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: use ipv4, SOCK_STREAM: tcp connection, 0: default protocal

    int enable = 1;
    setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // assigning and address to the socket
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;                // configures use of IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY -> server gets any IP address on the host machine (wifi/ethernet/etc) htonl -> converts byte order to uniform to work with any sys arch
    server_addr.sin_port = htons(PORT);              // set the port number

    bind(listen_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(listen_sock_fd, MAX_ACCEPT_BACKLOG);

    printf("[INFO]\tServer listening on port %d\n", PORT);

    struct sockaddr_in client_addr;
    int client_addr_len;

    int conn_sock_fd = accept(listen_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    printf("[INFO]\tClient connected to the server\n");

    while (1) {
        char buff[BUFF_SIZE];
        memset(buff, 0, BUFF_SIZE);

        int read_n = recv(conn_sock_fd, buff, sizeof(buff), 0);

        if (read_n <= 0) {
            // error
            printf("[INFO]\tClient disconnected, Closing server\n");
            close(conn_sock_fd);
            
            conn_sock_fd = accept(listen_sock_fd, (struct sockaddr*) &client_addr, &client_addr_len);
            printf("[INFO]\tNew Client connected to the server\n");
            continue;
        }
        printf("[CLIENT MESSAGE]\t %s", buff);
        strrev(buff);
        send(conn_sock_fd, buff, read_n, 0);
    }
}