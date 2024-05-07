#ifndef XPS_H
#define XPS_H

#define _GNU_SOURCE
// header files
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>


// 3rd party lib
#include "lib/vec/vec.h"

// consts
#define DEFAULT_BACKLOG 64
#define MAX_EPOLL_EVENTS 32
#define DEFAULT_BUFFER_SIZE 100000

// data types
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;

// structs

struct xps_listener_s;
struct xps_connection_s;

// struct typedefs
typedef struct xps_listener_s xps_listener_t;
typedef struct xps_connection_s xps_connection_t;

// function typedefs
typedef void (*xps_handler_t)(void *ptr);

// temp decls
extern vec_void_t listeners;
extern vec_void_t connections;
int xps_loop_create();
void xps_loop_attach(int epoll_fd, int fd, int events);
void xps_loop_detach(int epoll_fd, int fd);
void xps_loop_run(int epoll_fd);

// xps headers
#include "network/xps_listener.h"
#include "network/xps_connection.h"
#include "utils/xps_logger.h"
#include "utils/xps_utils.h"

#endif