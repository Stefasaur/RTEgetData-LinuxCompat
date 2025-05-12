/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file      com_lib.h
 * @brief     COM port library function declarations and definitions.
 * @author    B. Premzel
 */

#ifndef _COM_LIB_H
#define _COM_LIB_H

#define MAX_PORT_NAME_LEN  16

int  com_open(void);
void com_close(void);
int  com_read_memory(unsigned char* buffer, unsigned int address, unsigned int length);
int  com_write_memory(const unsigned char* buffer, unsigned address, unsigned length);
void com_flush(void);
void com_display_errors(const char* message);
const char* com_get_error_text(void);

#endif  // _COM_LIB_H

/*==== End of file ====*/
