/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/*******************************************************************************
 * @file    bridge.cpp
 * @author  B. Premzel
 * @brief   Bridge functions between the top app layer and driver interface.
 *          Each of the functions either calls the appropriate implementation 
 *          based on the active communication port (GDB, COM, ...) or reports
 *          an error if the functionality is not supported for the selected
 *          communication mode.
 ******************************************************************************/

#include "pch.h"
#include <stdint.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include "com_lib.h"
#include "gdb_lib.h"
#include "RTEgetData.h"
#include "logger.h"
#include "cmd_line.h"
#include "bridge.h"
#ifdef _WIN32
    #include <tlhelp32.h>
#endif


//*********** Local functions ***********
#ifdef _WIN32
static DWORD GetProcessIdByName(const char* processName);
static void set_process_priority(const char* process_name, DWORD dwPriorityClass, bool report_error);
#endif
static void decrease_priorities(void);
static void increase_priorities(void);


/**
 * @brief Opens the communication port based on the active interface.
 *
 * This function opens the communication port specified by the active interface.
 * It calls the appropriate connection function based on the active interface.
 * If the connection fails, and logging to a file is enabled, an error message is logged to the console.
 *
 * @return RTE_OK if the port is opened successfully, RTE_ERROR otherwise.
 */
int port_open(void)
{
    int ret_value = RTE_ERROR;
    bool log_to_file = logging_to_file();

    switch (parameters.active_interface)
    {
        case GDB_PORT:
            if (gdb_connect(parameters.gdb_port) != RTE_OK)
            {
                if (log_to_file)
                {
                    printf("Could not connect to the GDB server. Check the log file for details.\n");
                }

                return RTE_ERROR;
            }
            ret_value = RTE_OK;
            break;

        case COM_PORT:
            if (com_open() != RTE_OK)
            {
                if (log_to_file)
                {
                    printf("Could not open COM port: %s\n", parameters.com_port.name);
                }
                
                return RTE_ERROR;
            }
            ret_value = RTE_OK;
            break;

        default:
            break;
    }

    if (ret_value == RTE_OK)
    {
        increase_priorities();
    }

    return ret_value;
}


/**
 * @brief Closes the communication port based on the active interface.
 * 
 * This function attempts to close the communication port specified by the active interface.
 * It calls the appropriate implementation based on the active interface (GDB, COM, etc.).
 */

void port_close(void)
{
    decrease_priorities();

    switch (parameters.active_interface)
    {
        case GDB_PORT:
            gdb_detach();
            gdb_socket_cleanup();
            break;

        case COM_PORT:
            com_close();
            break;

        default:
            break;
    }
}


/**
 * @brief Reads data from the target device's memory using the active communication interface.
 *
 * This function reads a specified number of bytes from the target device memory starting at the
 * given address. The appropriate read function is called based on the currently active interface.
 *
 * @param buffer    Pointer to the buffer that will receive the read data. Must not be NULL.
 * @param address   Start address in the target memory to read from.
 * @param length    Number of bytes to read. Must be greater than 0.
 *
 * @return RTE_OK on success, RTE_ERROR on failure.
 *         Sets the global variable 'last_error' to provide additional error context.
 */

int port_read_memory(unsigned char* buffer, unsigned int address, unsigned int length)
{
    last_error = ERR_NO_ERROR;
    int res;

    log_data("\nReading %llu bytes ", (long long)length);
    log_data("from address 0x%08llX ", (long long)address);
    LARGE_INTEGER StartingTime;
    start_timer(&StartingTime);

    if ((length < 1U) || (buffer == NULL))
    {
        last_error = ERR_BAD_INPUT_DATA;
        return RTE_ERROR;
    }

    switch (parameters.active_interface)
    {
        case GDB_PORT:
            res = gdb_read_memory(buffer, address, length);
            break;

        case COM_PORT:
            res = com_read_memory(buffer, address, length);
            break;

        default:
            res = RTE_ERROR;
            break;
    }

    if (res == RTE_OK)
    {
        log_timing(" (%.1f ms)", &StartingTime);
    }

    return res;
}


/**
 * @brief Writes data to the target device's memory using the active communication interface.
 *
 * This function writes a specified number of bytes from the provided buffer to the target
 * device memory starting at the given address. It selects the appropriate write function
 * based on the currently active interface.
 *
 * @param buffer    Pointer to the buffer containing the data to write. Must not be NULL.
 * @param address   Start address in the target memory to write to.
 * @param length    Number of bytes to write. Must be greater than 0.
 *
 * @return RTE_OK on success, RTE_ERROR on failure.
 *         Sets the global variable 'last_error' to provide additional error context.
 */

int port_write_memory(const unsigned char* buffer, unsigned address, unsigned length)
{
    last_error = ERR_NO_ERROR;
    int res;

    log_data("\nWriting %llu bytes ", (long long)length);
    log_data("to address 0x%08llX ", (long long)address);
    LARGE_INTEGER StartingTime;
    start_timer(&StartingTime);

    if ((length < 1U) || (buffer == NULL))
    {
        last_error = ERR_BAD_INPUT_DATA;
        return RTE_ERROR;
    }

    switch (parameters.active_interface)
    {
        case GDB_PORT:
            res = gdb_write_memory(buffer, address, length);
            break;

        case COM_PORT:
            res = com_write_memory(buffer, address, length);
            break;

        default:
            res = RTE_ERROR;
            break;
    }

    if (res == RTE_OK)
    {
        log_timing(" (%.1f ms)", &StartingTime);
    }

    return res;
}


/**
 * @brief Flushes the active interface's communication channel.
 * 
 * This function flushes the communication channel of the active interface, ensuring that
 * all data is sent or received.
 * It calls the appropriate flush function based on the active interface (GDB, COM, etc.).
 */

void port_flush(void)
{
    switch (parameters.active_interface)
    {
        case GDB_PORT:
            gdb_flush_socket();
            break;

        case COM_PORT:
            com_flush();
            break;

        default:
            break;
    }
}


/**
 * @brief Handles unexpected messages received through the active interface.
 * 
 * This function is responsible for processing any unexpected messages that may be received
 * through the active interface, whether it be GDB or COM.
 * It calls the appropriate handling function based on the active interface.
 */

void port_handle_unexpected_messages(void)
{
    switch (parameters.active_interface)
    {
        case GDB_PORT:
            gdb_handle_unexpected_messages();
            break;

        case COM_PORT:
            com_flush();
            break;

        default:
            break;
    }
}


/**
 * @brief Execute a command accordingly to the active interface
 * 
 * @param command  Command string to execute
 * 
 * @return RTE_OK on success, RTE_ERROR otherwise
 */

int port_execute_command(const char* command)
{
    switch (parameters.active_interface)
    {
        case GDB_PORT:
            return gdb_execute_command(command);

        case COM_PORT:
            log_string("\nCommands only possible for a GDB server.%s", "");
            return RTE_ERROR;

        default:
            return RTE_ERROR;
    }
}


/**
 * @brief Reconnects to the previously established communication channel.
 *
 * This function attempts to re-establish communication with the target device.
 * It first resets process priorities to normal, then attempts to reconnect
 * based on the active interface:
 *   - GDB_PORT: Cleans up the existing socket and attempts to reconnect to the GDB server.
 *   - COM_PORT: Closes the current COM port and attempts to reopen it.
 *
 * If reconnection fails, an error message is displayed to the console if logging to a file is enabled.
 * Upon successful reconnection, process priorities are increased again to improve performance.
 *
 * @note This function assumes that a connection was previously established.
 */

void port_reconnect(void)
{
    decrease_priorities();
    printf("\n");

    switch (parameters.active_interface)
    {
        case GDB_PORT:
            gdb_socket_cleanup();

            if (gdb_connect(parameters.gdb_port) != RTE_OK)
            {
                if (logging_to_file())
                {
                    printf("\nCould not connect to the GDB server. Check the log file for details.\n");
                }
                return;
            }
            break;

        case COM_PORT:
            com_close();

            if (com_open() != RTE_OK)
            {
                if (logging_to_file())
                {
                    printf("\nCould not open COM port: %s", parameters.com_port.name);

#ifdef _WIN32
                    if (GetLastError())
                    {
                        printf(" (%s)", strerror(GetLastError()));
                    }
#else
                    if (errno)
                    {
                        printf(" (%s)", strerror(errno));
                    }
#endif

                    printf("\n");
                }
                return;
            }
            break;

        default:
            return;
    }

    increase_priorities();
    printf("\nOK\n");
}


/**
 * @brief Close all open files, clean up connection, and exit with return code 1.
 */

#ifdef _WIN32
    __declspec(noreturn)
#else
    __attribute__((noreturn))
#endif
void port_close_files_and_exit(void)
{
    decrease_priorities();              // Restore the normal priority

    switch (parameters.active_interface)
    {
        case GDB_PORT:
            gdb_detach();
            gdb_socket_cleanup();
            break;

        case COM_PORT:
            com_close();
            break;

        default:
            break;
    }

    if (parameters.log_file != NULL)
    {
        printf("\n\nAn error occurred during the transfer of data from the embedded system."
            "\nThe log file contains further details.\n\n");
    }

#ifdef _WIN32
    (void)_fcloseall();
#else
    // No direct equivalent on Linux, files are closed automatically
#endif
    exit(1);
}


/**
 * @brief Display error message if the logging is redirected to a log file.
 *        The errors are displayed accordingly to the active interface.
 *
 * @param message Text message to show in case of error.
 */

void port_display_errors(const char* message)
{
    switch (parameters.active_interface)
    {
        case GDB_PORT:
            gdb_display_errors(message);
            break;

        case COM_PORT:
            com_display_errors(message);
            break;

        default:
            break;
    }
}


/**
 * @brief Returns a pointer to the short error message text.
 *
 * @return Text message to show in case of error.
 */

const char* port_get_error_text(void)
{
    switch (parameters.active_interface)
    {
        case GDB_PORT:
            return gdb_get_error_text();

        case COM_PORT:
            return com_get_error_text();

        default:
            return "";
    }
}


/**
 * @brief Retrieves the process ID (PID) of a process by its name.
 *
 * This function searches for a running process with the specified name and returns its process ID.
 * It uses the Toolhelp32 API to enumerate running processes.
 *
 * @param processName The name of the process to search for (e.g., "notepad.exe").
 *                    This should be the executable file name without the path.
 *
 * @return The process ID (PID) of the found process, or 0 if the process is not found or an error occurs.
 */

#ifdef _WIN32
static DWORD GetProcessIdByName(const char* processName)
{
    if ((processName == NULL) || (processName[0] == '\0')) {
        return 0;
    }

    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    // Convert process name from char to wide char
    WCHAR appNameW[_MAX_PATH];
    errno_t err_code = mbstowcs_s(NULL, appNameW, _MAX_PATH, processName, _TRUNCATE);

    if (err_code != 0)
    {
        (void)CloseHandle(snapshot);
        return 0;
    }

    if (Process32FirstW(snapshot, &processEntry))
    {
        do
        {
            if (wcscmp(processEntry.szExeFile, appNameW) == 0)
            {
                (void)CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &processEntry));
    }

    (void)CloseHandle(snapshot);
    return 0;
}
#endif


/**
 * @brief Sets the priority class of a specified process.
 *
 * This function attempts to set the priority class of a process identified by its name.
 * It first retrieves the process ID (PID) using GetProcessIdByName. If the process is found,
 * it opens a handle to the process with the necessary permissions and then attempts to set
 * its priority class using SetPriorityClass.
 *
 * @param processName      The name of the process (e.g., "notepad.exe"). Must not be NULL or empty.
 * @param dwPriorityClass  The desired priority class (e.g., REALTIME_PRIORITY_CLASS, HIGH_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS).
 * @param report_error     If true, error messages are logged to the console if any step fails.
 *
 * @note The caller must have sufficient privileges to modify the priority of the target process.
 * @note If the process is not found or an error occurs, the function will log an error message if report_error is true.
 */

#ifdef _WIN32
static void set_process_priority(const char* processName, DWORD dwPriorityClass, bool report_error)
{
    DWORD processId = GetProcessIdByName(processName);

    if (processId == 0)
    {
        if (report_error)
        {
            log_string("\nProcess %s not found.", processName);
        }
        return;
    }

    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processId);

    if (hProcess == NULL)
    {
        if (report_error)
        {
            log_string("\nUnable to get handle for process %s.", processName);
            log_data(" Error: %lu", GetLastError());
        }
        return;
    }

    if (!SetPriorityClass(hProcess, dwPriorityClass))
    {
        if (report_error)
        {
            log_string("\nFailed to set priority for process %s.", processName);
            log_data(" Error: %lu", GetLastError());
        }
        (void)CloseHandle(hProcess);
        return;
    }

    (void)CloseHandle(hProcess);
}
#endif


/**
 * @brief Increases the priority of the RTEgetData process and specified driver processes.
 *
 * This function elevates the priority of the current process (RTEgetData) and any
 * processes specified via the `-driver` command-line argument. It only performs
 * these actions if the `elevated_priority` flag is enabled in the program's
 * parameters.
 *
 * If `elevated_priority` is true:
 *   - The RTEgetData process's priority is set to `REALTIME_PRIORITY_CLASS`.
 *     If this fails, an error is logged.
 *   - For each driver name provided in `parameters.driver_names`, the corresponding
 *     process's priority is set to `REALTIME_PRIORITY_CLASS`. Errors during this
 *     step are logged.
 *
 * @note `REALTIME_PRIORITY_CLASS` requires administrator privileges. If the application
 *       is not run with admin rights, the priority might not be set to REALTIME.
 *       In this case, the OS will likely set it to HIGH_PRIORITY_CLASS.
 */

static void increase_priorities(void)
{
    if (parameters.elevated_priority)
    {
#ifdef _WIN32
        // Increase RTEgetData process priority
        bool success = SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

        if (!success)
        {
            log_data("\nError setting RTEgetData priority: %llu.",
                (long long)GetLastError());
        }

        // Increase GDB server and other related process priorities
        for (size_t i = 0; i < parameters.number_of_drivers; i++)
        {
            set_process_priority(parameters.driver_names[i], REALTIME_PRIORITY_CLASS, true);
        }
#else
        // Priority setting not implemented on Linux
        log_string("Priority elevation not implemented on Linux", NULL);
#endif
    }
}


/**
 * @brief Restores process priorities to normal for RTEgetData and specified driver processes.
 *
 * This function lowers the priority of the current process (RTEgetData) and any
 * processes specified via the `-driver` command-line argument back to normal.
 * It only performs these actions if the `elevated_priority` flag is enabled in
 * the program's parameters.
 *
 * If `elevated_priority` is true:
 *   - The RTEgetData process's priority is set to `NORMAL_PRIORITY_CLASS`.
 *   - For each driver name provided in `parameters.driver_names`, the corresponding
 *     process's priority is set to `NORMAL_PRIORITY_CLASS`. Errors during this
 *     step are not reported to avoid cluttering the console during shutdown.
 *
 * @note This function is typically called during program shutdown or when a
 *       connection is being closed or re-established.
 */

static void decrease_priorities(void)
{
    if (parameters.elevated_priority)
    {
#ifdef _WIN32
        // Restore RTEgetData process priority to normal.
        (void)SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

        // Restore driver process priorities to normal.
        for (size_t i = 0; i < parameters.number_of_drivers; i++)
        {
            set_process_priority(parameters.driver_names[i], NORMAL_PRIORITY_CLASS, false);
        }
#else
        // Priority setting not implemented on Linux
#endif
    }
}

/*==== End of file ====*/
