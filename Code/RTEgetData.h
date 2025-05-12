/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file      RTEgetData.h
 * @author    B. Premzel
 */

#ifndef _RTEGETDATA_H
#define _RTEGETDATA_H

#define RTEGETDATA_VERSION "v1.00"

#define MIN_BUFFER_SIZE  (64U + 16U)    // Minimum buffer size for g_rtedbg circular buffer
#define MAX_BUFFER_SIZE  2100000U       // Maximum buffer size for g_rtedbg circular buffer
#define MESSAGE_FILTER_ADDRESS  (parameters.start_address + offsetof(rtedbg_header_t, filter))
                                        // Address of the message filter
#define RTE_CFG_WORD_ADDRESS    (parameters.start_address + offsetof(rtedbg_header_t, rte_cfg))
                                        // Address of the RTE configuration word

#define MAX_DRIVERS 5                   // Maximum number of drivers that should get elevated execution priority
#define BENCHMARK_REPEAT_COUNT 1000     // Maximum number of data transfers in the benchmark
#define MAX_BENCHMARK_TIME_MS 20000     // Maximum time for the data transfer benchmark in milliseconds

// COM port communication parameters
#define COM_RX_BUFFER_SIZE 16384
#define COM_TX_BUFFER_SIZE 4096
#define COM_MAX_WRITE_MEMORY_SIZE 4     // Only a single word write to the g_rtedbg structure is supported
#define COM_DEFAULT_RX_TIMEOUT   50     // Default waiting time on echo for the single wire communication
#define COM_BAD_RESPONSE_DELAY   30     // Delay after bad response has been received from COM port

#define RTE_ERROR   1
#define RTE_OK      0

#define RTECOM_MAX_RECV_LEN     (65536U - 16U)  // Maximal packet length while receiving data over a COM port

#define DEFAULT_COM_RX_TIMEOUT    50    // Defalt receive timeout after the command has been sent to embedded system
#define DEFAULT_COM_BAUDRATE    9600    // Default COM baudrate


/*----------------------------------------------------
 *  E R R O R   C O D E S
 *
 * Update the error reporting functions if new error
 * codes are added to the enum.
 *---------------------------------------------------
 */
typedef enum _err_code
{
    ERR_NO_ERROR,

    // Common error codes
    ERR_RCV_TIMEOUT,                // Timeout - message has not been received
    ERR_SEND_TIMEOUT,               // Timeout - message could not be sent
    ERR_BAD_INPUT_DATA,             // Bad function parameter

    // GDB error codes
    ERR_SOCKET,                     // Winsock error
    ERR_BAD_MSG_FORMAT,             // Bad message format
    ERR_BAD_MSG_CHECKSUM,           // Bad message checksum
    ERR_RUN_LENGTH_ENCODING_NOT_IMPLEMENTED,
    ERR_CONNECTION_CLOSED,          // Socket has been closed
    ERR_MSG_NOT_SENT_COMPLETELY,    // The send() function could not send the complete message
    ERR_BAD_RESPONSE,               // Unknown/bad response from GDB
    ERR_GDB_REPORTED_ERROR,         // GDB server returned error message '$Exx#xx' or '$E.errtext#xx'

    // COM port communication error codes
    ERR_COM_CANNOT_OPEN_PORT,
    ERR_COM_RECEIVE,                // Frame, overrun, parity error, ...
    ERR_COM_BUFFER_OVERRUN
} err_code_t;


extern err_code_t last_error;

void initialize_data_logging_structure(unsigned cmd_word, unsigned timestamp_frequency);
void set_new_filter_value(const char* filter_value);
long clock_ms(void);

#endif  // _RTEGETDATA_H

/*==== End of file ====*/
