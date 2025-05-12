/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file   gdb_defs.h
 * @brief  Compile time parameters and error code definitions.
 * @author B. Premzel
 */


#ifndef __GDB_DEFS_H
#define __GDB_DEFS_H

#define DEFAULT_HOST_ADDRESS  "127.0.0.1"  // "localhost"

#define RECV_TIMEOUT           500      // Max. time in ms to receive a message from the GDB server
#define LONG_RECV_TIMEOUT     2500      // Max. time in ms to receive a query message from the GDB server
#define DEFAULT_SEND_TIMEOUT    50      // Max. time in ms to send a message to the GDB server
                                        // The send() function blocks only if no buffer space is available
                                        // within the transport system to hold the data to be transmitted.
#define ERROR_DATA_TIMEOUT      50      // Max. time in ms to wait for a message following the 'O' type error message

#define DEFAULT_MESSAGE_SIZE  4096      // Default max. send message size (sent to the GDB server) if there is
                                        // no 'PacketSize' field in the capability data

#define TCP_BUFF_LENGTH      65535      // The maximum TCP packet size including header

#endif  //__GDB_DEFS_H

/*==== End of file ====*/
