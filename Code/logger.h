/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file    logger.h
 * @author  B. Premzel
 * @brief   Header file for logging functions.
 */


#pragma once
#include <stdio.h>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <Windows.h>
#else
    #include <sys/time.h>
    #include <time.h>
    #include <stdint.h>
    typedef struct { int64_t QuadPart; } LARGE_INTEGER;
#endif

void enable_logging(bool on_off);
void create_log_file(const char* file_name);
void start_timer(LARGE_INTEGER * start_timer);
void log_data(const char * text, long long int data);
void log_string(const char * text, const char * string);
void log_timing(const char * text, LARGE_INTEGER * start);
void log_wsock_error(const char * text);
double time_elapsed(LARGE_INTEGER * start_timer);
void log_communication_text(const char* direction, const char* msg, int length);
void log_communication_hex(const char* direction, const char* msg, int length);
bool logging_to_file(void);
void disable_enable_logging_to_file(void);


/*==== End of file ====*/
