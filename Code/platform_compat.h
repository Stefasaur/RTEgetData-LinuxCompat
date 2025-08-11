/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file    platform_compat.h
 * @brief   Cross-platform compatibility functions
 * @author  B. Premzel
 */

#pragma once

#include <stdio.h>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <conio.h>
    #include <windows.h>
    #define kbhit() _kbhit()
    #define getch() _getch()
    #define sleep_ms(ms) Sleep(ms)
    typedef int errno_t;
#else
    // Linux implementations
    #include <unistd.h>
    #include <sys/time.h>
    int kbhit(void);
    int getch(void);
    #define sleep_ms(ms) usleep((ms) * 1000)
    typedef int errno_t;
    
    // Cross-platform file functions
    errno_t fopen_s(FILE** file, const char* filename, const char* mode);
    int sscanf_s(const char* str, const char* format, ...);
    int strerror_s(char* buffer, size_t size, int errnum);
    int sprintf_s(char* buffer, size_t size, const char* format, ...);
    
    // Cross-platform serial port constants
    #define NOPARITY    0
    #define ODDPARITY   1
    #define EVENPARITY  2
    #define ONESTOPBIT  0
    #define TWOSTOPBITS 2
#endif

/*==== End of file ====*/