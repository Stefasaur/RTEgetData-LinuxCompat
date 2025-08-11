/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file      gdb_lib.h
 * @brief     GDB library function declarations and definitions.
 * @author    B. Premzel
 */

#ifdef _WIN32
    #define _WINSOCK_DEPRECATED_NO_WARNINGS 1
    #include <winsock2.h>
    #include <Ws2tcpip.h>
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <errno.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif
#include <time.h>
#include "gdb_defs.h"

extern char message_buffer[];
extern clock_t app_start_time;

int  gdb_connect(unsigned short gdb_port);
int  gdb_read_memory(unsigned char * buffer, unsigned int address, unsigned int length);
int  gdb_write_memory(const unsigned char * buffer, unsigned address, unsigned length);
void gdb_detach(void);
int  gdb_execute_command(const char * command);
void gdb_flush_socket(void);
void gdb_socket_cleanup(void);
void gdb_handle_unexpected_messages(void);
void gdb_display_errors(const char* message);
const char* gdb_get_error_text(void);

/*==== End of file ====*/
