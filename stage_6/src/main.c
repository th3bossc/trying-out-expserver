#include "xps.h"

int epoll_fd;
struct epoll_event events[MAX_EPOLL_EVENTS];
vec_void_t listeners;
vec_void_t connections;

int main() {
    epoll_fd = xps_loop_create();

    vec_init(&listeners);
    vec_init(&connections);

    for (int port = 8001; port <= 8003; port++) {
        xps_listener_create(epoll_fd, "0.0.0.0", port);
        logger(LOG_INFO, "main()", "Server listening on port %u", port);
    }

    xps_loop_run(epoll_fd);
}

int xps_loop_create() {
    return epoll_create1(0);
}

void xps_loop_attach(int epoll_fd, int fd, int events) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

void xps_loop_detach(int epoll_fd, int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void xps_loop_run(int epoll_fd) {
    while(1) {
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait");
        int n_ready_fds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait over");

        for (int i = 0; i < n_ready_fds; i++) {
            int curr_fd = events[i].data.fd;

            xps_listener_t* listener = NULL;
            for (int j = 0; j < listeners.length; j++) {
                xps_listener_t* curr_listener = listeners.data[j];
                if (curr_listener->sock_fd == curr_fd) {
                    listener = curr_listener;
                    break;
                }
            }

            if (listener) {
                xps_listener_connection_handler(listener);
                continue;
            }


            xps_connection_t* connection = NULL;
            for (int j = 0; j < connections.length; j++) {
                xps_connection_t* curr_connection = connections.data[j];
                if (curr_connection->sock_fd == curr_fd) {
                    connection = curr_connection;
                    break;
                }
            }

            if (connection) {
                xps_connection_read_handler(connection);
            }
        }
    }
}