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
#include <signal.h>

// 3rd party lib
#include "lib/vec/vec.h"

// consts
#define DEFAULT_BACKLOG 64
#define MAX_EPOLL_EVENTS 32
#define DEFAULT_BUFFER_SIZE 100000
#define DEFAULT_NULL_THRESH 32

// error constants
#define OK               0  // SUCCESS
#define E_FAIL          -1  // Un-recoverable error 
#define E_AGAIN         -2  // Try again
#define E_NEXT          -3  // Do next
#define E_NOUTFOUND     -4  // File not found
#define E_PERMISSION    -5  // File permission denied
#define E_EOF           -6  // End of file reached

// data types
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;

// structs
struct xps_core_s;
struct xps_loop_s;
struct xps_listener_s;
struct xps_connection_s;

// struct typedefs
typedef struct xps_core_s xps_core_t;
typedef struct xps_loop_s xps_loop_t;
typedef struct xps_listener_s xps_listener_t;
typedef struct xps_connection_s xps_connection_t;

// function typedefs
typedef void (*xps_handler_t)(void *ptr);

// xps headers
#include "core/xps_core.h"
#include "core/xps_loop.h"
#include "network/xps_listener.h"
#include "network/xps_connection.h"
#include "utils/xps_logger.h"
#include "utils/xps_utils.h"

#endif