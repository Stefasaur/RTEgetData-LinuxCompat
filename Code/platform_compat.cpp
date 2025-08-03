/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file    platform_compat.cpp
 * @brief   Cross-platform compatibility functions implementation
 * @author  B. Premzel
 */

#include "platform_compat.h"

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

/***
 * @brief Check if a key has been pressed (non-blocking)
 * @return 1 if key is available, 0 otherwise
 */
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

/***
 * @brief Get a character from stdin (blocking)
 * @return The character pressed
 */
int getch(void)
{
    struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return ch;
}

/***
 * @brief Cross-platform fopen_s implementation
 */
errno_t fopen_s(FILE** file, const char* filename, const char* mode)
{
    if (file == NULL || filename == NULL || mode == NULL) {
        return EINVAL;
    }
    
    *file = fopen(filename, mode);
    if (*file == NULL) {
        return errno;
    }
    
    return 0;
}

/***
 * @brief Cross-platform sscanf_s implementation
 */
int sscanf_s(const char* str, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsscanf(str, format, args);
    va_end(args);
    return result;
}

/***
 * @brief Cross-platform strerror_s implementation
 */
int strerror_s(char* buffer, size_t size, int errnum)
{
    if (buffer == NULL || size == 0) {
        return EINVAL;
    }
    
    const char* msg = strerror(errnum);
    strncpy(buffer, msg, size - 1);
    buffer[size - 1] = '\0';
    
    return 0;
}

/***
 * @brief Cross-platform sprintf_s implementation
 */
int sprintf_s(char* buffer, size_t size, const char* format, ...)
{
    if (buffer == NULL || size == 0 || format == NULL) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, size, format, args);
    va_end(args);
    
    if (result >= (int)size) {
        buffer[size - 1] = '\0';
        return -1;  // Buffer too small
    }
    
    return result;
}

#endif

/*==== End of file ====*/