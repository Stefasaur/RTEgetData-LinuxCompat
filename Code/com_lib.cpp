/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file      com_lib.cpp
 * @brief     COM port library functions.
 * @author    B. Premzel
 */


#include <stdio.h>
#include <stdlib.h>
#include "logger.h"
#include "com_lib.h"
#include "cmd_line.h"
#include "rte_com.h"
#include "RTEgetData.h"

#ifdef _WIN32
#include <cstring>
#include <cctype>

//********** Global variables ***********
HANDLE h_com_port;              // COM port handle

//*********** Local functions ***********
static void com_resynchronize(void);
static int com_send(const char* p_data, DWORD length);
static void com_purge_and_log(void);
static void com_log_error(DWORD errors);
static int com_receive(unsigned char* buffer, unsigned length, const char* type);
static int com_send_command(uint8_t command, uint32_t address, uint32_t data);
static int com_read_memory_block(unsigned char* buffer, unsigned int address, unsigned int length);
static int com_write_memory_block(const unsigned char* buffer, unsigned address, unsigned length);
static bool prepare_com_port_name(const char* input_port, wchar_t* output_buffer, size_t buffer_size);
static int check_response(char command);
static void log_api_error(const char* text);
const char* get_error_message_text(DWORD error_code);


/**
 * @brief Prepares the COM port name for use with the Windows API.
 *
 * This function takes a COM port name (e.g., "COM1") and converts it to the format
 * required by the Windows API (e.g., "\\.\COM1"). It also performs validation
 * to ensure the input is a valid COM port name and that the output buffer is large enough.
 *
 * @param input_port    The COM port name as a null-terminated string (e.g., "COM1").
 * @param output_buffer A pointer to a wide character buffer where the formatted COM port name will be stored.
 * @param buffer_size   The size of the output buffer in wide characters.
 * 
 * @return true if the COM port name was successfully prepared, false otherwise.
 */

static bool prepare_com_port_name(const char* input_port, wchar_t* output_buffer, size_t buffer_size)
{
    // Check for null parameters
    if ((input_port == NULL) || (output_buffer == NULL) || (buffer_size == 0))
    {
        return false;
    }

    // Verify input starts with "COM"
    if (strncmp(input_port, "COM", 3) != 0)
    {
        return false;
    }

    // Calculate required buffer size (including null terminator)
    // Format will be: \\.\COMx
    // Need 4 chars for \\.\ + length of original COM name + null terminator
    size_t required_size = 4 + strlen(input_port) + 1;

    // Check if buffer is large enough
    if (buffer_size < required_size)
    {
        return false;
    }

    swprintf_s(output_buffer, buffer_size, L"\\\\.\\%S", input_port);
    return true;
}


/**
 * @brief Opens the specified COM port and configures it with the desired settings.
 *
 * This function opens the COM port specified in the `parameters.com_port.name`
 * global variable. It configures the port with the baud rate, data bits, stop bits,
 * and parity specified in the `parameters.com_port` structure. It also sets up
 * timeouts and FIFO buffers for the port.
 *
 * @return RTE_OK if the COM port was opened and configured successfully, RTE_ERROR otherwise.
 */

int com_open(void)
{
    last_error = ERR_COM_CANNOT_OPEN_PORT;

    wchar_t port_name[MAX_PORT_NAME_LEN];
    size_t len = strlen(parameters.com_port.name);
    
    for (size_t i = 0; i < len; i++)
    {
        parameters.com_port.name[i] = (char)toupper(parameters.com_port.name[i]);
    }

    bool ok = prepare_com_port_name(parameters.com_port.name, port_name, MAX_PORT_NAME_LEN);

    if (!ok)
    {
        log_string("Incorrect COM port name: %s", parameters.com_port.name);
        return RTE_ERROR;
    }

    log_string("Open port %s: ", parameters.com_port.name);

    // Attempt to open COM port
    h_com_port = CreateFile(
        port_name,                      // Port name
        GENERIC_READ | GENERIC_WRITE,   // Read/Write access
        0,                              // No sharing
        NULL,                           // No security attributes
        OPEN_EXISTING,                  // Must use OPEN_EXISTING
        0,                              // Normal file attribute
        NULL                            // No template
    );

    // Check if port was opened successfully
    if (h_com_port == INVALID_HANDLE_VALUE)
    {
        log_string("Could not open COM port: %s", parameters.com_port.name);
        log_api_error(" - ");
        return RTE_ERROR;
    }

    // Configure DCB structure for COM port parameters
    DCB dcb_serial_params = { 0 };
    dcb_serial_params.DCBlength = sizeof(dcb_serial_params);

    // Get current DCB settings
    if (!GetCommState(h_com_port, &dcb_serial_params))
    {
        log_string("Could not retrieve COM port settings", NULL);
        log_api_error(" - ");
        CloseHandle(h_com_port);
        return RTE_ERROR;
    }

    // Set COM port parameters
    dcb_serial_params.BaudRate = parameters.com_port.baudrate;
    dcb_serial_params.ByteSize = 8;
    dcb_serial_params.StopBits = parameters.com_port.stop_bits;
    dcb_serial_params.Parity = parameters.com_port.parity;
    dcb_serial_params.fParity = (parameters.com_port.parity == NOPARITY) ? FALSE : TRUE;

    // Apply new DCB settings
    if (!SetCommState(h_com_port, &dcb_serial_params))
    {
        log_string("Could not change COM port settings", NULL);
        log_api_error(" - ");
        CloseHandle(h_com_port);
        return RTE_ERROR;
    }

    /* Set the timeouts so that ReadFile returns immediately if data is available,
     * waits for one byte if not, and times out if none arrives within the specified time.
     * See: https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts (Remarks)
     */
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = parameters.com_port.recv_start_timeout;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;

    if (!SetCommTimeouts(h_com_port, &timeouts))
    {
        log_string("Error SetComTimeouts", NULL);
        log_api_error(" - ");
        CloseHandle(h_com_port);
        return RTE_ERROR;
    }

    // Set receive and transmit FIFO sizes
    if (!SetupComm(h_com_port, COM_RX_BUFFER_SIZE, COM_TX_BUFFER_SIZE))
    {
        log_string("Could not set COM port FIFO size", NULL);
        log_api_error(" - ");
        CloseHandle(h_com_port);
        return RTE_ERROR;
    }

    // Purge any existing data in the buffers
    PurgeComm(h_com_port, PURGE_RXCLEAR | PURGE_TXCLEAR);
    log_string("OK", NULL);
    last_error = ERR_NO_ERROR;

    return RTE_OK;
}


/**
 * @brief Closes the currently open COM port.
 *
 * This function closes the COM port handle `h_com_port` if it is valid.
 * It also flushes any remaining data in the buffers before closing.
 */

void com_close(void)
{
    if ((h_com_port != NULL) && (h_com_port != INVALID_HANDLE_VALUE))
    {
        com_flush();
        CloseHandle(h_com_port);
    }
}


/**
 * @brief Sends data over the COM port.
 *
 * This function sends the specified data over the COM port. It logs the data
 * being sent and checks for errors during the write operation.
 *
 * @param p_data A pointer to the data to be sent.
 * @param length The length of the data to be sent in bytes.
 * 
 * @return RTE_OK if the data was sent successfully, RTE_ERROR otherwise.
 */

static int com_send(const char* p_data, DWORD length)
{
    if ((h_com_port == INVALID_HANDLE_VALUE) || (p_data == NULL) || (length == 0))
    {
        last_error = ERR_BAD_INPUT_DATA;
        return RTE_ERROR;
    }

    DWORD bytes_written = 0;
    log_communication_hex("Send", p_data, length);
    bool ok = WriteFile(h_com_port, p_data, length, &bytes_written, NULL);

    // Check if write was successful and all bytes were written
    if ((!ok) || (bytes_written != length))
    {
        log_api_error("Write to COM port error");

        // Clear any error state
        PurgeComm(h_com_port, PURGE_TXABORT | PURGE_TXCLEAR);
        last_error = ERR_SEND_TIMEOUT;
        return RTE_ERROR;
    }

    // Ensure all data is transmitted before returning
    if (!FlushFileBuffers(h_com_port))
    {
        log_api_error("Flush COM buffer error");
        last_error = ERR_SEND_TIMEOUT;
        return RTE_ERROR;
    }

    return RTE_OK;
}


/**
 * @brief Discards any unexpected data received on the COM port.
 *
 * This function checks for and discards any data that has been received on the
 * COM port but was not expected. It also logs the unexpected data.
 */

static void com_purge_and_log(void)
{
    if (h_com_port == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD bytes_read = 0;
    COMSTAT com_stat;
    DWORD errors;

    // Clear any error state and get COM status
    if (!ClearCommError(h_com_port, &errors, &com_stat))
    {
        return;
    }

    // Check if there's data available to read
    if (com_stat.cbInQue == 0)
    {
        return;     // No data available = OK
    }

    // Limit read size to available data or buffer size and read data
    unsigned char data[40];
    DWORD bytes_to_read = (DWORD)min(sizeof(data), com_stat.cbInQue);
    bool success = ReadFile(h_com_port, &data, bytes_to_read, &bytes_read, NULL);
    
    if (!success)
    {
        log_api_error("Read from COM port error");
    }

    if ((bytes_read > 0) && success)
    {
        log_communication_hex("Unexpected data received", (const char *)data, bytes_read);

        if (com_stat.cbInQue > bytes_read)
        {
            log_data("... + %u bytes", (long long)(com_stat.cbInQue - bytes_read));
            com_flush();        // Flush the remaining data
        }
    }

    // Clear any error state
    PurgeComm(h_com_port, PURGE_RXABORT | PURGE_RXCLEAR);
}


/**
 * @brief Logs any COM port errors that have occurred.
 *
 * This function logs the specific COM port errors that have been detected.
 *
 * @param errors A bitmask of COM port error flags.
 */

static void com_log_error(DWORD errors)
{
    if (errors == 0)
    {
        return;
    }

    last_error = ERR_COM_RECEIVE;
    const char* separator = NULL;

    log_string("\nCOM error: ", NULL);

    if (errors & CE_BREAK)
    {
        log_string("break", NULL);
        separator = ", ";
    }

    if (errors & CE_FRAME)
    {
        log_string("%sframming", separator);
        separator = ", ";
    }

    if (errors & CE_OVERRUN)
    {
        log_string("%sbuffer overrun", separator);
        last_error = ERR_COM_BUFFER_OVERRUN;
        separator = ", ";
    }

    if (errors & CE_RXOVER)
    {
        log_string("%soverflow", separator);
        separator = ", ";
    }

    if (errors & CE_RXPARITY)
    {
        log_string("%sparity", separator);
    }
}


/**
 * @brief Receives data from the COM port.
 *
 * This function receives a specified number of bytes from the COM port. It uses
 * overlapped I/O to handle asynchronous reads and supports timeouts.
 *
 * @param buffer A pointer to the buffer where the received data will be stored.
 * @param size   The number of bytes to receive.
 * @param type   A string describing the type of data being received (for logging).
 * 
 * @return RTE_OK if the data was received successfully, RTE_ERROR otherwise.
 */

static int com_receive(unsigned char* buffer, unsigned size, const char* type)
{
    unsigned total_received = 0U;
    clock_t recv_start_time = clock();

    if ((h_com_port == INVALID_HANDLE_VALUE) || (buffer == NULL) || (size == 0))
    {
        last_error = ERR_BAD_INPUT_DATA;
        return RTE_ERROR;
    }

    while (total_received < size)
    {
        DWORD bytes_read = 0;
        DWORD read_size = size - total_received;

        if (!ReadFile(h_com_port, buffer + total_received, read_size, &bytes_read, NULL))
        {
            log_api_error("Read from COM port error");
            return RTE_ERROR;
        }

        total_received += bytes_read;

        if (bytes_read == 0)
        {
            break;
        }
    }

    if (total_received > 0)
    {
        DWORD errors;
        COMSTAT comStat;

        if (ClearCommError(h_com_port, &errors, &comStat))
        {
            com_log_error(errors);
        }

        log_communication_hex(type, (const char *)buffer, total_received);
    }

    if (total_received < size)
    {
        last_error = ERR_RCV_TIMEOUT;
        log_data(" timeout after %u ms ", clock() - recv_start_time);
    }

    return (total_received == size) ? RTE_OK : RTE_ERROR;
}


/**
 * @brief Attempts to resynchronize the communication with the embedded system.
 *
 * This function sends a sequence of 0xFF characters to the embedded system to
 * try to force it into a known state. It also handles the echo in single-wire
 * communication mode.
 */

static void com_resynchronize(void)
{
    int result = com_send((const char*)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 10);

    if ((result == RTE_OK) && parameters.com_port.single_wire_communication)
    {
        unsigned char echo[RTECOM_SEND_PACKET_LEN];
        (void)com_receive(echo, RTECOM_SEND_PACKET_LEN, "Echo");
    }

    com_flush();
}


/**
 * @brief Sends a command to the embedded system via the COM port.
 *
 * This function sends a command packet to the embedded system, including the
 * command, address, data, and checksum. It also handles the echo in single-wire
 * communication mode.
 *
 * @param command The command to be sent.
 * @param address The address associated with the command.
 * @param data    The data associated with the command.
 * 
 * @return RTE_OK if the command was sent successfully, RTE_ERROR otherwise.
 */

static int com_send_command(uint8_t command, uint32_t address, uint32_t data)
{
    rtecom_send_data_t message = { 0 };
    message.command = command;
    message.address = address;
    message.data = data;            // Write = data, Read = length

    // Calculate the checksum
    uint8_t checksum = RTECOM_CHECKSUM;
    uint8_t *msg = (uint8_t *)&message.address;

    for (int i = 0; i < 8; i++)
    {
        checksum ^= *msg++;
    }

    message.checksum = checksum;
    com_purge_and_log();            // There should be no data in the input buffer
    int result = com_send((char*)&message.command, RTECOM_SEND_PACKET_LEN);

    if (result != RTE_OK)
    {
        return RTE_ERROR;
    }

    // In single-wire communication, it also receives all characters sent.
    // The echo must be the same as the data sent.
    if (parameters.com_port.single_wire_communication)
    {
        unsigned char echo[RTECOM_SEND_PACKET_LEN] = { 0 };
        result = com_receive(echo, RTECOM_SEND_PACKET_LEN, "Echo");

        if (result != RTE_OK)
        {
            log_string("Bad or no echo", NULL);
            com_flush();
            return RTE_ERROR;
        }

        if (memcmp(echo, &message.command, RTECOM_SEND_PACKET_LEN) != 0)
        {
            Sleep(COM_BAD_RESPONSE_DELAY);
            log_string("\nBad echo  ", NULL);
            com_flush();
            return RTE_ERROR;
        }
    }

    return  RTE_OK;
}


/**
 * @brief Reads a block of memory from the embedded system via the COM port.
 *
 * This function sends a read command to the embedded system and then receives
 * the requested memory block.
 *
 * @param buffer  A pointer to the buffer where the received data will be stored.
 * @param address The starting address of the memory block to be read.
 * @param length  The length of the memory block to be read in bytes.
 * 
 * @return RTE_OK if the memory block was read successfully, RTE_ERROR otherwise.
 */

static int com_read_memory_block(unsigned char* buffer, unsigned int address, unsigned int length)
{
    if ((length > RTECOM_MAX_RECV_LEN) || (length == 0U))
    {
        return RTE_ERROR;
    }

    int rez = com_send_command(RTECOM_READ_RTEDBG, address, length);

    if (rez != RTE_OK)
    {
        return RTE_ERROR;
    }

    return com_receive(buffer, length, "Recv");
}


/**
 * @brief Reads memory from the embedded system via the COM port.
 *
 * This function reads memory from the embedded system in blocks, handling
 * cases where the requested length exceeds the maximum receive length.
 *
 * @param buffer A pointer to the buffer where the received data will be stored.
 * @param address The starting address of the memory to be read.
 * @param length The total length of the memory to be read in bytes.
 * 
 * @return RTE_OK if the memory was read successfully, RTE_ERROR otherwise.
 */

int com_read_memory(unsigned char* buffer, unsigned int address, unsigned int length)
{
    // Maximal packet length while receiving data
    unsigned max_size = min(parameters.max_message_size, RTECOM_MAX_RECV_LEN);
    unsigned size = 0;

    for (; length > 0; length -= size)
    {
        size = length > max_size ? max_size : length;
        int rez = com_read_memory_block(buffer, address, size);
        buffer += size;
        address += size;

        if (rez != RTE_OK)
        {
            com_resynchronize();
            return RTE_ERROR;
        }
    }

    return RTE_OK;
}


/**
 * @brief Writes a block of memory to the embedded system via the COM port.
 *
 * This function sends a write command to the embedded system to write a block
 * of memory. The address of the data transferred to the embedded system is
 * that of a 32-bit word, rather than a byte. The current protocol implementation
 * only allows one word to be sent per data packet.
 *
 * @param buffer A pointer to the buffer containing the data to be written.
 * @param address The starting address of the memory to be written.
 * @param length The length of the memory block to be written in bytes.
 * 
 * @return RTE_OK if the memory block was written successfully, RTE_ERROR otherwise.
 */

static int com_write_memory_block(const unsigned char* buffer, unsigned address, unsigned length)
{
    if ((length != 4U) || (address & 3U))
    {
        return RTE_ERROR;
    }

    // Address is address of word to be written and not byte (divide by 4)
    int ret = com_send_command(RTECOM_WRITE_RTEDBG, address / 4U, *((uint32_t *)buffer));

    if (ret != RTE_OK)
    {
        return RTE_ERROR;
    }

    return check_response(RTECOM_WRITE_RTEDBG);
}


/**
 * @brief Writes memory to the embedded system via the COM port.
 *
 * This function writes memory to the embedded system in blocks of 4 bytes,
 * ensuring that the total length is a multiple of 4.
 *
 * @param buffer A pointer to the buffer containing the data to be written.
 * @param address The starting address of the memory to be written.
 * @param length The total length of the memory to be written in bytes.
 * 
 * @return RTE_OK if the memory was written successfully, RTE_ERROR otherwise.
 */

int com_write_memory(const unsigned char* buffer, unsigned address, unsigned length)
{
    if (length & 3U)
    {
        log_data("\nWrite memory length (%u) must be divisible by 4.", length);
        return RTE_ERROR;
    }

    if (address & 3U)
    {
        log_data("\nWrite address length (0x%X) must be divisible by 4.", address);
        return RTE_ERROR;
    }

    clock_t last_time = clock();

    for (unsigned i = 0U; i < length; i += 4U)
    {
        if (length > 4)
        {
            clock_t new_time = clock();

            if ((new_time - last_time) > 99)
            {
                putchar('.');
                last_time = new_time;
            }
        }

        int ret = com_write_memory_block(buffer, address, 4U);

        if (ret != RTE_OK)
        {
            com_resynchronize();
            return RTE_ERROR;
        }
        address += 4U;
        buffer += 4U;
    }

    return RTE_OK;
}


/**
 * @brief Purge any existing data in the COM port communication buffers.
 */

void com_flush(void)
{
    if (h_com_port != INVALID_HANDLE_VALUE)
    {
        PurgeComm(h_com_port, PURGE_RXCLEAR | PURGE_TXCLEAR);
    }
}


/**
 * @brief Wait for response from embedded system
 * 
 * The embedded system returns ACK if the index is within the g_rtedbg structure
 * and the command (e.g. data write) has been performed, otherwise NACK (see below).
 * No response is sent if the command was not received correctly (e.g. bad checksum).
 * 
 * @param commmand - command sent to embedded system is response if the data/address
 *                   value is not correct (NACK value).
 * 
 * @return RTE_OK if embedded system responded with ACK, RTE_ERROR otherwise.
 */

static int check_response(char command)
{
    int ret_val = RTE_ERROR;
    clock_t recv_start_time = clock();

    if (h_com_port == INVALID_HANDLE_VALUE)
    {
        return RTE_ERROR;
    }

    DWORD bytes_read = 0;
    unsigned char data;
    bool success = ReadFile(h_com_port, &data, 1, &bytes_read, NULL);

    if (success)
    {
        if (bytes_read > 0)
        {
            if (data == RTECOM_ACK)
            {
                ret_val = RTE_OK;
            }
            else if (data == command)
            {
                log_string(" NACK received ", NULL);
            }
            else
            {
                log_data(" Bad response 0x%02X ", data);
                com_purge_and_log();
            }
        }
        else
        {
            last_error = ERR_RCV_TIMEOUT;
            log_data(" timeout after %u ms ", clock() - recv_start_time);
        }
    }
    else
    {
        log_api_error("Read from COM port error");
    }

    return ret_val;
}


/**
 * @brief Returns a pointer to the short error message text.
 *        Trailing spaces are used to overwrite previous message(s).
 *
 * @return Text message to show in case of error.
 */

const char* com_get_error_text(void)
{
    switch (last_error)
    {
        case ERR_COM_CANNOT_OPEN_PORT:
            return "COM port closed           ";

        case ERR_COM_BUFFER_OVERRUN:
            return "buffer overrun            ";

        case ERR_RCV_TIMEOUT:
            return "receive timeout           ";

        case ERR_COM_RECEIVE:
            return "receive error             ";

        case ERR_SEND_TIMEOUT:
            return "send timeout              ";

        default:
            return "                          ";
    }
}


/**
 * @brief Display error message if the logging is redirected to a log file.
 *
 * @param message Text message to show in case of error.
 */

void com_display_errors(const char* message)
{
    if (!logging_to_file() || (last_error == 0))
    {
        printf("\n");
        return;
    }

    printf(message);
    switch (last_error)
    {
        case ERR_COM_CANNOT_OPEN_PORT:
            printf("cannot open port %s", parameters.com_port.name);
            break;

        case ERR_COM_BUFFER_OVERRUN:
            printf("buffer overrun");
            break;

        case ERR_RCV_TIMEOUT:
            printf("receive timeoout");
            break;

        case ERR_COM_RECEIVE:
            printf("receive error");
            break;

        case ERR_SEND_TIMEOUT:
            printf("send timeout");
            break;

        case ERR_BAD_INPUT_DATA:
            printf("bad function parameter");
            break;

        default:
            break;
    }

    last_error = ERR_NO_ERROR;
    printf("\nCheck the log file for details.\n");
}


/**
 * @brief Logs the last Windows API error with context text.
 *
 * @param text Context message to print before the error.
 */

static void log_api_error(const char* text)
{
    DWORD error = GetLastError();

    if (!error || (text == NULL))
    {
        return;
    }

    log_string("\n%s: ", text);
    const char* error_text = get_error_message_text(error);

    if (error_text == NULL)
    {
        log_string("unknown error ", NULL);
    }
    else
    {
        log_string("%s", error_text);
    }
}


/**
 * @brief Formats an error message for the specified error code.
 *
 * This function formats an error message for the given error code using
 * FormatMessage() to retrieve the system error message. The message is stored
 * in a static buffer, which means it will be overwritten with each call.
 *
 * @param error_code The error code (typically from GetLastError())
 * 
 * @return Pointer to a static buffer containing the formatted error message
 */

const char* get_error_message_text(DWORD error_code)
{
    // Static buffer to hold the error message
    static char error_buffer[512];

    DWORD length;
    LPVOID temp_buffer = NULL;

    // Get the error message from the system
    length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&temp_buffer,
        0, NULL);

    if (length == 0)
    {
        // FormatMessage failed
        snprintf(error_buffer, sizeof(error_buffer),
            "Unknown error code: %lu (0x%08lX)", error_code, error_code);
    }
    else
    {
        // Copy the message to the static buffer
        // Trim trailing whitespace and newlines
        while (length > 0 && isspace(((LPSTR)temp_buffer)[length - 1]))
        {
            length--;
        }

        if (length >= sizeof(error_buffer))
        {
            length = sizeof(error_buffer) - 1;
        }

        memcpy(error_buffer, temp_buffer, length);
        error_buffer[length] = '\0';

        // Free the buffer allocated by FormatMessage
        LocalFree(temp_buffer);
    }

    return error_buffer;
}

#else  // Linux implementation

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstring>
#include "platform_compat.h"

//********** Global variables ***********
static int serial_fd = -1;              // Serial port file descriptor
static char linux_error_buffer[256];    // Error message buffer

//*********** Local functions ***********
static speed_t get_baud_rate(int baud);
static int set_serial_attributes(int fd);
static void log_linux_error(const char* operation);

/***
 * @brief Convert integer baud rate to speed_t constant
 */
static speed_t get_baud_rate(int baud)
{
    switch (baud) {
        case 9600:    return B9600;
        case 19200:   return B19200;
        case 38400:   return B38400;
        case 57600:   return B57600;
        case 115200:  return B115200;
        case 230400:  return B230400;
        case 460800:  return B460800;
        case 500000:  return B500000;
        case 576000:  return B576000;
        case 921600:  return B921600;
        case 1000000: return B1000000;
        case 1152000: return B1152000;
        case 1500000: return B1500000;
        case 2000000: return B2000000;
        case 2500000: return B2500000;
        case 3000000: return B3000000;
        case 3500000: return B3500000;
        case 4000000: return B4000000;
        default:      return B9600;  // Default fallback
    }
}

/***
 * @brief Configure serial port attributes
 */
static int set_serial_attributes(int fd)
{
    struct termios tty;
    
    if (tcgetattr(fd, &tty) != 0) {
        log_linux_error("tcgetattr");
        return RTE_ERROR;
    }

    // Set baud rate
    speed_t speed = get_baud_rate(parameters.com_port.baudrate);
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    // Configure for raw mode
    tty.c_cflag &= ~PARENB;  // No parity
    tty.c_cflag &= ~CSTOPB;  // One stop bit
    tty.c_cflag &= ~CSIZE;   // Clear data size bits
    tty.c_cflag |= CS8;      // 8 data bits
    tty.c_cflag &= ~CRTSCTS; // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines

    // Configure parity based on parameters
    switch (parameters.com_port.parity) {
        case ODDPARITY:
            tty.c_cflag |= PARENB | PARODD;
            break;
        case EVENPARITY:
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            break;
        case NOPARITY:
        default:
            tty.c_cflag &= ~PARENB;
            break;
    }

    // Configure stop bits
    if (parameters.com_port.stop_bits == TWOSTOPBITS) {
        tty.c_cflag |= CSTOPB;
    } else {
        tty.c_cflag &= ~CSTOPB;
    }

    // Input flags - turn off input processing
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

    // Output flags - turn off output processing
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    // Control characters
    tty.c_cc[VTIME] = parameters.com_port.recv_start_timeout / 100; // Wait timeout in deciseconds
    tty.c_cc[VMIN] = 0;    // Return immediately with any received data

    // Local flags
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw mode

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        log_linux_error("tcsetattr");
        return RTE_ERROR;
    }

    return RTE_OK;
}

/***
 * @brief Log Linux system error with context
 */
static void log_linux_error(const char* operation)
{
    snprintf(linux_error_buffer, sizeof(linux_error_buffer), 
             "%s failed: %s", operation, strerror(errno));
    log_string(linux_error_buffer, NULL);
}

/***
 * @brief Open and configure serial port
 */
int com_open(void)
{
    char device_path[64];
    
    // Convert COM port name to Linux device path
    // COM1 -> /dev/ttyS0, COM2 -> /dev/ttyS1, etc.
    // Also support direct device paths like /dev/ttyUSB0
    if (strncmp(parameters.com_port.name, "COM", 3) == 0) {
        int port_num = atoi(&parameters.com_port.name[3]);
        if (port_num > 0) {
            snprintf(device_path, sizeof(device_path), "/dev/ttyS%d", port_num - 1);
        } else {
            log_string("Invalid COM port number", NULL);
            return RTE_ERROR;
        }
    } else if (parameters.com_port.name[0] == '/') {
        // Direct device path (e.g., /dev/ttyUSB0, /dev/ttyACM0)
        strncpy(device_path, parameters.com_port.name, sizeof(device_path) - 1);
        device_path[sizeof(device_path) - 1] = '\0';
    } else {
        // Assume it's a device name without /dev/ prefix
        snprintf(device_path, sizeof(device_path), "/dev/%s", parameters.com_port.name);
    }

    log_string("Opening serial port: ", device_path);

    // Open serial port
    serial_fd = open(device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial_fd < 0) {
        log_linux_error("open");
        return RTE_ERROR;
    }

    // Configure serial port
    if (set_serial_attributes(serial_fd) != RTE_OK) {
        close(serial_fd);
        serial_fd = -1;
        return RTE_ERROR;
    }

    // Flush any existing data
    tcflush(serial_fd, TCIOFLUSH);

    log_string(" - OK", NULL);
    return RTE_OK;
}

/***
 * @brief Close serial port
 */
void com_close(void)
{
    if (serial_fd >= 0) {
        close(serial_fd);
        serial_fd = -1;
        log_string("Serial port closed", NULL);
    }
}

/***
 * @brief Read data from embedded system via serial port
 */
int com_read_memory(unsigned char* buffer, unsigned int address, unsigned int length)
{
    if (serial_fd < 0) {
        log_string("Serial port not open", NULL);
        return RTE_ERROR;
    }

    // Send read command to embedded system
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "R%08X%04X\n", address, length);
    
    if (write(serial_fd, cmd, strlen(cmd)) < 0) {
        log_linux_error("write");
        return RTE_ERROR;
    }

    // Read response
    ssize_t bytes_read = read(serial_fd, buffer, length);
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            log_string("Read timeout", NULL);
            return RTE_ERROR;  // Use RTE_ERROR instead of RTE_TIMEOUT
        }
        log_linux_error("read");
        return RTE_ERROR;
    }

    if (bytes_read != (ssize_t)length) {
        log_data("Expected %lld bytes, got different amount", (long long)length);
        return RTE_ERROR;
    }

    return RTE_OK;
}

/***
 * @brief Write data to embedded system via serial port
 */
int com_write_memory(const unsigned char* buffer, unsigned address, unsigned length)
{
    if (serial_fd < 0) {
        log_string("Serial port not open", NULL);
        return RTE_ERROR;
    }

    // Send write command followed by data
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "W%08X%04X", address, length);
    
    if (write(serial_fd, cmd, strlen(cmd)) < 0) {
        log_linux_error("write command");
        return RTE_ERROR;
    }
    
    if (write(serial_fd, buffer, length) < 0) {
        log_linux_error("write data");
        return RTE_ERROR;
    }

    return RTE_OK;
}

/***
 * @brief Flush serial port buffers
 */
void com_flush(void)
{
    if (serial_fd >= 0) {
        tcflush(serial_fd, TCIOFLUSH);
    }
}

/***
 * @brief Display COM port error message
 */
void com_display_errors(const char* message)
{
    if (message) {
        printf("%s", message);
    }
    
    if (serial_fd < 0) {
        printf(" (Serial port not open)");
    }
}

/***
 * @brief Get COM port error text
 */
const char* com_get_error_text(void)
{
    if (serial_fd < 0) {
        return "Serial port not open";
    }
    
    return linux_error_buffer[0] ? linux_error_buffer : "No error";
}

#endif  // _WIN32

/*==== End of file ====*/
