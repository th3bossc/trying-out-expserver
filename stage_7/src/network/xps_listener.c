#include "../xps.h"

void listener_connection_handler(void* ptr);

xps_listener_t* xps_listener_create(xps_core_t* core, const char* host, u_int port) {
    assert(host != NULL);
    assert(is_valid_port(port));

    int sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (sock_fd < 0) {
        logger(LOG_ERROR, "xps_listener_create()", "socket() failed");
        perror("Error message");
        return NULL;
    }

    const int enable = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        logger(LOG_ERROR, "xps_listener_create()", "setsockopt() failed");
        perror("Error message");
        close(sock_fd);
        return NULL;
    }

    struct addrinfo* addr_info = xps_getaddrinfo(host, port);
    if (addr_info == NULL) {
        logger(LOG_ERROR, "xps_listener_create()", "xps_getaddrinfo() failed");
        close(sock_fd);
        return NULL;
    }

    if (bind(sock_fd, addr_info->ai_addr, addr_info->ai_addrlen) < 0) {
        logger(LOG_ERROR, "xps_listener_create()", "bind() failed");
        perror("Error message");
        freeaddrinfo(addr_info);
        close(sock_fd);
        return NULL;
    }

    freeaddrinfo(addr_info);

    if (listen(sock_fd, DEFAULT_BACKLOG) < 0) {
        logger(LOG_ERROR, "xps_listener_create()", "listen() failed");
        perror("Error message");
        close(sock_fd);
        return NULL;
    }

    xps_listener_t* listener = malloc(sizeof(xps_listener_t));
    if (listener == NULL) {
        logger(LOG_ERROR, "xps_listener_create()", "malloc() failed");
        close(sock_fd);
        return NULL;
    }

    listener->core = core;
    listener->port = port;
    listener->sock_fd = sock_fd;
    listener->host = host;

    xps_loop_attach(listener->core->loop, sock_fd, EPOLLIN, listener, listener_connection_handler);
    vec_push(&core->listeners, listener);

    logger(LOG_DEBUG, "xps_listener_create()", "created listener on port %d", port);

    return listener;
}

void xps_listener_destroy(xps_listener_t* listener) {

    assert(listener != NULL);

    xps_loop_detach(listener->core->loop, listener->sock_fd);

    vec_void_t listeners = listener->core->listeners;
    for (int i = 0; i < listeners.length; i++) {
        xps_listener_t* current = listeners.data[i];
        if (current != NULL && current == listener) {
            listeners.data[i] = NULL;
            break;
        }
    }

    close(listener->sock_fd);


    logger(LOG_DEBUG, "xps_listener_destroy()", "destroyed listener on port %d", listener->port);
    free(listener);
}


void listener_connection_handler(void* ptr) {
    assert(ptr != NULL);

    xps_listener_t* listener = ptr;

    struct sockaddr conn_addr;
    socklen_t conn_addr_len = sizeof(conn_addr);
    int conn_sock_fd = accept(listener->sock_fd, &conn_addr, &conn_addr_len);

    if (conn_sock_fd < 0) {
        logger(LOG_ERROR, "xps_listener_connection_handler()", "accept() failed");
        perror("Error message");
        return;
    }

    xps_connection_t* client = xps_connection_create(listener->core, conn_sock_fd);

    if (client == NULL) {
        logger(LOG_ERROR, "xps_listener_connection_handler()", "xps_connection_create() failed");
        close(conn_sock_fd);
        return;
    }

    client->listener = listener;
    logger(LOG_INFO, "xps_listener_connection_handler()", "new connection");
}
