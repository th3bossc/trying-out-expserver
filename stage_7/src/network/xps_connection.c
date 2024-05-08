#include "../xps.h"

void reverse_string(char* input, long len) {
    for (int start = 0, end = len-1; start < end; start++, end--) {
        char temp = input[start];
        input[start] = input[end];
        input[end] = temp;
    }
}


void connection_read_handler(void* ptr);

xps_connection_t* xps_connection_create(xps_core_t* core, int sock_fd) {
    xps_connection_t *connection = malloc(sizeof(xps_connection_t));

    if (connection == NULL) {
        logger(LOG_ERROR, "xps_connection_create()", "malloc() failed");
        perror("Error message");
        return NULL;
    } 

    // xps_loop_attach(epoll_fd, sock_fd, EPOLLIN);
    xps_loop_attach(core->loop, sock_fd, EPOLLIN, connection, connection_read_handler);


    connection->core = core;
    connection->sock_fd = sock_fd;
    connection->remote_ip = get_remote_ip(sock_fd);
    connection->listener = NULL;

    vec_push(&core->connections, connection);

    logger(LOG_DEBUG, "xps_connection_create()", "created connection");

    return connection;
}

void xps_connection_destroy(xps_connection_t* connection) {
    assert(connection != NULL);

    vec_void_t connections = connection->core->connections;
    for (int i = 0; i < connections.length; i++) {
        xps_connection_t* current = connections.data[i];

        if (current != NULL && current == connection) {
            connections.data[i] = NULL;
            break;
        }
    }

    xps_loop_detach(connection->core->loop, connection->sock_fd);
    close(connection->sock_fd);
    free(connection->remote_ip);
    free(connection);
    logger(LOG_DEBUG, "xps_connection_destroy()", "connection destroyed");
}

void connection_read_handler(void* ptr) {
    assert(ptr != NULL);

    xps_connection_t* connection = ptr;

    char buff[DEFAULT_BUFFER_SIZE];
    long read_n = recv(connection->sock_fd, buff, sizeof(buff), 0);

    if (read_n < 0) {
        logger(LOG_ERROR, "xps_connection_read_handler()", "recv() failed");
        perror("Error message");
        xps_connection_destroy(connection);
        return;
    }

    if (read_n == 0) {
        logger(LOG_INFO, "xps_connection_read_handler()", "peer closed connection");
        xps_connection_destroy(connection);
        return;
    }

    buff[read_n] = 0;

    reverse_string(buff, read_n);

    long bytes_written = 0;
    long message_len = read_n;

    while(bytes_written < message_len) {
        long write_n = send(connection->sock_fd, buff + bytes_written, message_len - bytes_written, 0);
        if (write_n < 0) {
            logger(LOG_ERROR, "xps_connection_read_handler()", "send() failed");
            perror("Error message");
            xps_connection_destroy(connection);
            return;
        }
        bytes_written += write_n;
    }

}