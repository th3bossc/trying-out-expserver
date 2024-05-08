#include "../xps.h"

bool is_valid_port(u_int port) {
    return port >= 0 && port <= 65535;
}


struct addrinfo* xps_getaddrinfo(const char* host, u_int port) {
    assert(host != NULL);
    assert(is_valid_port(port));

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[20];
    sprintf(port_str, "%u", port);

    int err = getaddrinfo(host, port_str, &hints, &result);

    if (err != 0) {
        logger(LOG_ERROR, "xps_getaddrinfo()", "getaddrinfo() failed");
        return NULL;
    }

    char ip_str[30];
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)result->ai_addr;
    if (inet_ntop(result->ai_family, &(ipv4->sin_addr), ip_str, sizeof(ip_str)) == NULL) {
        logger(LOG_ERROR, "xps_getaddrinfo()", "inet_ntop() failed");
        perror("Error message");
        freeaddrinfo(result);
        return NULL;
    }

    logger(LOG_DEBUG, "xps_getaddrinfo()", "host: %s, port %u, resolved ip %s", host, port, ip_str);

    return result;
}

int make_socket_non_blocking(u_int sock_fd) {
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags < 0) {
        logger(LOG_ERROR, "make_socket_non_blocking()", "failed to get flags");
        perror("Error message");
        return E_FAIL;
    }

    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        logger(LOG_ERROR, "make_socket_non_blocking()", "failed to set flags");
        perror("Error message");
        return E_FAIL;
    }

    return OK;
}

char* get_remote_ip(u_int sock_fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char ipstr[INET_ADDRSTRLEN];

    if (getpeername(sock_fd, (struct sockaddr*) &addr, &addr_len) != 0) {
        logger(LOG_ERROR, "get_remote_ip()", "getpeername() failed");
        perror("Error messsage");
        return NULL;
    }

    char* ip_str = malloc(INET_ADDRSTRLEN);
    if (ip_str == NULL) {
        logger(LOG_ERROR, "get_remote_ip()", "malloc() failed");
        perror("Error message");
        return NULL;
    }

    inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);

    return ip_str;
}

void vec_filter_null(vec_void_t* v) {
    assert(v != NULL);

    vec_void_t temp;
    vec_init(&temp);

    for (int i = 0; i < v->length; i++) {
        void* curr = v->data[i];
        if (curr != NULL)
            vec_push(&temp, curr);
    }

    vec_clear(v);
    for (int i = 0; i < temp.length; i++) {
        void* curr = temp.data[i];
        vec_push(v, curr);
    }

    vec_deinit(&temp);
}


