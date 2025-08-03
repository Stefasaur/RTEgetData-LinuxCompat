/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

 /*********************************************************************************
  * @file    rte_com.h
  * @author  Branko Premzel
  * @brief   Definition of a communication protocol with an embedded system over a
  *          serial channel.
  *          See the [RTEcomLib](https://github.com/RTEdbg/RTEcomLib) repository
  *          for implementation details. 
  *******************************************************************************/

#ifndef _RTE_COM_H
#define _RTE_COM_H

#include <stdint.h>


typedef struct
{
    uint16_t no_bytes;
    uint8_t command;        // Command - this is the first byte received from the host
    uint8_t checksum;       // The checksum is the XOR of 0x0F and the last eight bytes of
                            // the message (address and data word)
    uint32_t address;       // Address of data buffer to be transferred or variable to be set
    uint32_t data;          // Size of data (length) or data written to the address
} rtecom_send_data_t;

typedef enum
{
    // The first two commands are mandatory.
    RTECOM_WRITE_RTEDBG,    // Write 32-bit data to g_rtedbg (e.g. set message filter or index)
                            // Address = index of 32-bit word
                            // Returns ACK if index is within g_rtedbg header, NACK otherwise
    RTECOM_READ_RTEDBG,     // Read data from g_rtedbg data structure
                            // Address is relative to the start of the g_rtgdb data structure.
                            // Returns: NN bytes (NN = data parameter � number of bytes requested)
                            //          or NACK if requested data not inside of g_rtedbg
                            // Optional commands
    RTECOM_READ,            // Get data from the specified address ('data' = number of bytes)
                            // Returns: requested amount data or nothing if address not OK
    RTECOM_WRITE32,         // Write 32-bit data to the specified address
                            // Returns: ACK
    RTECOM_WRITE16,         // Write 16-bit data to the specified address
                            // Returns: ACK
    RTECOM_WRITE8,          // Write 8-bit data to the specified address
                            // Returns: ACK
    RTECOM_LAST_COMMAND
} rte_com_command_t;

#define RTECOM_CHECKSUM  0x0FU  // Initial checksum value
#define RTECOM_ACK  RTECOM_CHECKSUM

// Legend: ACK - acknowledge - sends RTECOM_CHECKSUM
//         NACK - sends the command value
// Note: NACK is returned if the command (e.g. RTECOM_READ) is not implemented or address not OK.

// Host always sends 10 bytes: command (8b), checksum (8b), address (32b), data (32b)
#define RTECOM_SEND_PACKET_LEN  10U

// Maximum length of data block received from embedded system
#define MAX_COM_RECEIVE_MSG_SIZE  65520

#endif  // _RTE_COM_H

/*==== End of file ====*/
