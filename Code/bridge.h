/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

 /*******************************************************************************
 * @file    bridge.h
 * @author  B. Premzel
 * @brief   Bridge functions between the top app layer and communication port 
 *          driver.
 ******************************************************************************/

#ifndef _BRIDGE_H
#define _BRIDGE_H

int port_open(void);
void port_close(void);
int port_read_memory(unsigned char* buffer, unsigned int address, unsigned int length);
int port_write_memory(const unsigned char* buffer, unsigned address, unsigned length);
void port_flush(void);
void port_handle_unexpected_messages(void);
void port_reconnect(void);
__declspec(noreturn) void port_close_files_and_exit(void);
void port_display_errors(const char* message);
int port_execute_command(const char* command);
const char* port_get_error_text(void);

#endif  // _BRIDGE_H

/*==== End of file ====*/
