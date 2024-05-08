#include "../xps.h"

void reverse_string(char* input, long len) {
    for (int start = 0, end = len-1; start < end; start++, end--) {
        char temp = input[start];
        input[start] = input[end];
        input[end] = temp;
    }
}


void connection_read_handler(void* ptr);
void connection_write_handler(void* ptr);
void connection_close_handler(void* ptr);

xps_connection_t* xps_connection_create(xps_core_t* core, u_int sock_fd) {
    xps_connection_t *connection = malloc(sizeof(xps_connection_t));

    if (connection == NULL) {
        logger(LOG_ERROR, "xps_connection_create()", "malloc() failed for 'connection'");
        perror("Error message");
        return NULL;
    } 

    xps_buffer_list_t* write_buff_list = xps_buffer_list_create();
    if (write_buff_list == NULL) {
        logger(LOG_ERROR, "xps_connection_create()", "xps_buffer_list_create() failed");
        free(connection);
        return NULL;
    }

    connection->core = core;
    connection->sock_fd = sock_fd;
    connection->remote_ip = get_remote_ip(sock_fd);
    connection->listener = NULL;
    connection->write_buff_list = write_buff_list;


    // xps_loop_attach(epoll_fd, sock_fd, EPOLLIN);
    xps_loop_attach(core->loop, sock_fd, EPOLLIN | EPOLLOUT, connection, connection_read_handler, connection_write_handler, connection_close_handler);

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
        logger(LOG_ERROR, "connection_read_handler()", "recv() failed");
        perror("Error message");
        xps_connection_destroy(connection);
        return;
    }

    if (read_n == 0) {
        logger(LOG_INFO, "connection_read_handler()", "peer closed connection");
        xps_connection_destroy(connection);
        return;
    }

    buff[read_n] = 0;

    logger(LOG_DEBUG, "connection_read_handler()", "received text: %s", buff);
    reverse_string(buff, read_n);

    xps_buffer_t* write_buffer = xps_buffer_create(read_n, read_n, NULL);
    memcpy(write_buffer->data, buff, read_n);

    xps_buffer_list_append(connection->write_buff_list, write_buffer);
}

void connection_write_handler(void* ptr) {
    assert(ptr != NULL);

    xps_connection_t* connection = ptr;

    if (connection->write_buff_list->len == 0)
        return;

    xps_buffer_t* write_buffer = xps_buffer_list_read(connection->write_buff_list, connection->write_buff_list->len);
    if (write_buffer == NULL) {
        logger(LOG_ERROR, "connection_write_handler()", "xps_buffer_list_read() failed");
        return;
    }


    long write_n = send(connection->sock_fd, write_buffer->data, write_buffer->len, 0);
    if (write_n == -1 && errno == EAGAIN || errno == EWOULDBLOCK) {
        logger(LOG_ERROR, "connection_write_handler()", "send() failed");
        perror("Error message");
        xps_connection_destroy(connection);
        return;
    }
    if (write_n > 0)
        xps_buffer_list_clear(connection->write_buff_list, write_n);
}


void connection_close_handler(void* ptr) {
    assert(ptr != NULL);

    xps_connection_t* connection = ptr;

    logger(LOG_INFO, "connection_close_handler()", "peer closed connection");
    xps_connection_destroy(connection);
}