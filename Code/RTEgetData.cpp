/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file  RTEgetData.cpp
 * 
 * @brief The utility enables data transfer from the embedded system to the host using a GDB server
 *        or a COM port. See the Readme.md file for a detailed description, limitations, workarounds,
 *        and instructions for use.
 *
 * @note Code has been tested with the J-LINK, ST-LINK, and OpenOCD GDB servers. 
 *       The program does not automatically try to restart the data transfer if it
 *       was not successful. The data transfer has to be restarted by the user.
 *
 * @author B. Premzel
 */


#include "pch.h"
#ifdef _WIN32
    #pragma comment(lib, "Ws2_32.lib")   // Link additional libraries
    #include <malloc.h>
#else
    #include <stdlib.h>
#endif
#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <cerrno>
#include <cctype>
#include "bridge.h"
#include "cmd_line.h"
#include "rtedbg.h"
#include "logger.h"
#include "platform_compat.h"



//********** Global variables ***********
parameters_t parameters;             // Command line parameters
uint32_t old_msg_filter;             // Filter value before data logging is disabled
rtedbg_header_t rtedbg_header;       // Header of the g_rtedbg structure loaded from embedded system
static unsigned* p_rtedbg_structure; // Pointer to memory area allocated for the g_rtedbg structure
err_code_t last_error;               // Last error detected


//*********** Local functions ***********
static bool allocate_memory_for_g_rtedbg_structure(void);
static void benchmark_data_transfer(void);
static int  check_header_info(void);
static bool data_logging_disabled(void);
static void delay_before_data_transfer(void);
static void display_logging_state(clock_t* start_time);
static void execute_decode_batch_file(void);
static int  erase_buffer_index(void);
static int  execute_commands_from_file(const char* cmd_file);
static void execute_commands_from_file_x(char name_start);
static void internal_command(const char* cmd_text);
static int  load_rtedbg_structure_header(void);
static int  pause_data_logging(void);
static int  persistent_connection(void);
static void print_filter_info(void);
static void print_rtedbg_header_info(void);
static int  read_memory_block(unsigned char * buffer, uint32_t address, uint32_t size);
static void repeat_start_command_file(void);
static int  reset_circular_buffer(void);
static int  save_rtedbg_structure(void);
static int  set_or_restore_message_filter(void);
static bool single_shot_active(void);
static int  single_data_transfer(void);
static void show_help(void);
static void switch_to_post_mortem_logging(void);
static void switch_to_single_shot_logging(void);
static void load_and_display_rtedbg_structure_header(void);


/***
 * @brief Main function
 * 
 * @param argc Number of command line parameters (including the APP full path name)
 * @param argv Array of pointers to the command line parameter strings
 *
 * @return 0 - OK
 *         1 - error occurred
 */

int main(int argc, char * argv[])
{
    int rez;
    clock_t main_start_time = clock_ms();
    process_command_line_parameters(argc, argv);

    if (port_open() != RTE_OK)
    {
        return 1;
    }

    rez = execute_commands_from_file(parameters.start_cmd_file);
    
    if (rez != RTE_OK)
    {
        port_close();
#ifdef _WIN32
    #ifdef _WIN32
    (void)_fcloseall();
#else
    // No direct equivalent on Linux, files are closed automatically
#endif
#else
        // No direct equivalent on Linux, files are closed automatically
#endif
        return 1;
    }
    
    if (parameters.persistent_connection)
    {
        rez = persistent_connection();
        printf("\n");
    }
    else
    {
        rez = single_data_transfer();
        log_data("\nTotal time: %llu ms\n\n", (long long)(clock_ms() - main_start_time));

        if (logging_to_file() && (rez != RTE_OK))
        {
            port_display_errors("\nFailed to read data from the embedded system:");
        }
    }

    port_close();
#ifdef _WIN32
    (void)_fcloseall();
#else
    // No direct equivalent on Linux, files are closed automatically
#endif
    return rez;
    }


/***
 * @brief Execute a single data transfer and exit
 *
 * @return RTE_OK    - no error
 *         RTE_ERROR - error occurred (data not received)
 */

static int single_data_transfer(void)
{
    if (logging_to_file())
    {
        printf("\nReading from embedded system... ");
    }

    port_handle_unexpected_messages();

    // Read the current message filter value before turning off filtering.
    int rez = port_read_memory((unsigned char*)&old_msg_filter, MESSAGE_FILTER_ADDRESS, 4U);

    if (rez != RTE_OK)
    {
        return RTE_ERROR;
    }

    // Pause data logging if the old message filter is not zero.
    if (old_msg_filter != 0)
    {
        if (pause_data_logging() != RTE_OK)
        {
            return RTE_ERROR;
        }
    }

    if (load_rtedbg_structure_header() != RTE_OK)
    {
        return RTE_ERROR;
    }

    if (check_header_info() != RTE_OK)
    {
        return RTE_ERROR;
    }

    if (save_rtedbg_structure() != RTE_OK)
    {
        (void)set_or_restore_message_filter();
        return RTE_ERROR;
    }

    if (logging_to_file())
    {
        printf("\nData written to \"%s\"\n", parameters.bin_file_name);
    }

    if (!data_logging_disabled())
    {
        // Data logging has been enabled by the firmware already
        set_or_restore_message_filter();
        log_string("\nThe data logging has already been enabled by the firmware.\n", NULL);

        if (logging_to_file())
        {
            printf("\nThe data logging has already been enabled by the firmware.\n");
        }

        return RTE_ERROR;
    }

    if (reset_circular_buffer() != RTE_OK)
    {
        if (logging_to_file())
        {
            printf("\nCircular buffer in g_rtedbg structure not properly cleared!");
        }
    }

    if (set_or_restore_message_filter() != RTE_OK)
    {
        return RTE_ERROR;
    }

    // Execute the decode batch file if specified.
    execute_decode_batch_file();

    return RTE_OK;
}


/***
 * @brief Execute the -decode=name batch file if the command line argument was defined.
 */

static void execute_decode_batch_file(void)
{
    if (parameters.decode_file != NULL)
    {
        printf("\nStarting the batch file: %s", parameters.decode_file);
        int rez = system(parameters.decode_file);

        if (rez != 0)
        {
            printf("\nThe '%s' batch file could not be started!", parameters.decode_file);
        }
        else
        {
            printf("\n");
        }
    }
}


/***
 * @brief Display a list of commands and associated keys.
 */

static void show_help(void)
{
    printf(
        "\n\nAvailable commands:"
        "\n   'Space' - Start data transfer and decoding if the -decode=decode_batch_file argument is used."
        "\n   'F' - Set new filter value."
        "\n   'S' - Switch to single shot mode and restart logging."
        "\n   'P' - Switch to post-mortem mode and restart logging."
        "\n   'R' - Reconnect to the GDB server or COM port."
        "\n   '0' - Restart the batch file defined with the -start argument."
        "\n   '1' ... '9' - Start the command file 1.cmd ... 9.cmd. "
        "\n   'B' - Benchmark data transfer speed."
        "\n   'H' - Load the data logging structure header and display information."
        "\n   'L' - Enable / disable logging to the log file."
        "\n   '?' - View an overview of available commands."
        "\n   'Esc' - Exit."
        "\n----------------------------------------------------------------------"
        "\n"
    );
}


/***
 * @brief Load and display the g_rtedbg structure header information.
 *        This function first loads the header, checks its validity,
 *        and then prints the header information if valid.
 */

static void load_and_display_rtedbg_structure_header(void)
{
    int rez;
    rez = load_rtedbg_structure_header();

    if (rez != RTE_OK)
    {
        return;
    }

    rez = check_header_info();

    if (rez != RTE_OK)
    {
        printf("\nIncorrect header info (incorrect address or rte_init() not executed).");
        return;
    }

    print_rtedbg_header_info();
}


/***
 * @brief Print names of enabled message filters or their numbers if
 *        the filter name file is not available.
 */

static void print_filter_info(void)
{
    if (rtedbg_header.filter == 0)
    {
        printf("\nMessage filter is zero (data logging disabled).");
        return;
    }

    FILE* filters = NULL;

    if (parameters.filter_names)
    {
#ifdef _WIN32
        errno_t err = fopen_s(&filters, parameters.filter_names, "r");

        if (err != 0)
        {
            char error_text[256];
#ifdef _WIN32
            (void)_strerror_s(error_text, sizeof(error_text), NULL);
#else
            strerror_s(error_text, sizeof(error_text), errno);
#endif
            printf("\nCannot open \"%s\" file. Error: %s", parameters.filter_names, error_text);
            port_close_files_and_exit();
        }
#else
        filters = fopen(parameters.filter_names, "r");

        if (filters == NULL)
        {
            char error_text[256];
            strerror_r(errno, error_text, sizeof(error_text));
            printf("\nCannot open \"%s\" file. Error: %s", parameters.filter_names, error_text);
            port_close_files_and_exit();
        }
#endif
    }

    uint32_t filter = rtedbg_header.filter;
    printf("\nEnabled message filters (0x%08X): ", filter);
    bool filter_number_printed = false;

    // Iterate through each bit in the filter to check which filters are enabled
    for (uint32_t i = 0; i < 32U; i++)
    {
        char filter_name[256U];
        filter_name[0] = '\0';

        // If filter names are provided, read the corresponding name from the file
        if (parameters.filter_names)
        {
            (void)fgets(filter_name, sizeof(filter_name), filters);
            char* newline = strchr(filter_name, '\n');

            if (newline != NULL)
            {
                *newline = '\0';    // Remove the newline character
            }

            // Print the filter info only if the filter is enabled and the name is defined
            if (((filter & 0x80000000UL) != 0) && (*filter_name != '\0'))
            {
                printf("\n%2u - %s", i, filter_name);
            }
        }
        else
        {
            // Print the filter index if no filter names are provided
            if ((filter & 0x80000000UL) != 0)
            {
                if (filter_number_printed)
                {
                    printf(", ");
                }
                printf("%u", i);
                filter_number_printed = true;
            }
        }

        filter <<= 1U; // Shift the filter to check the next bit
    }

    if (filters != NULL)
    {
        (void)fclose(filters);
    }
}


/***
 * @brief Check if the message filter value is zero. It was set to zero before
 *        the data transfer and should remain zero until the transfer is complete.
 *        Report an error if it is not.
 *
 * @return true  - message filter is zero (data logging disabled)
 *         false - message filter could not be checked or is not zero
 */

static bool data_logging_disabled(void)
{
    uint32_t message_filter;
    int rez = port_read_memory((unsigned char*)&message_filter, MESSAGE_FILTER_ADDRESS, 4U);

    if (rez != RTE_OK)
    {
        return false;
    }

    if (message_filter != 0)
    {
        printf(
            "\n\nError: At the beginning of the transfer, the message filter was"
            "\nset to 0 to allow uninterrupted data transfer to the host."
            "\nAt the end of the data transfer, the message filter is not zero."
            "\nApparently, the filter was enabled by the firmware. Data "
            "\ntransferred from the embedded system may be partially corrupted.\n"
            );
        return false;
    }

    return true;
}


/***
 * @brief Switch to single shot logging mode. The single shot mode must be
 *        enabled in the firmware.
 */

static void switch_to_single_shot_logging(void)
{
    if (load_rtedbg_structure_header() != RTE_OK)
    {
        return;
    }

    if (!RTE_SINGLE_SHOT_LOGGING_ENABLED)
    {
        printf("\nSingle shot logging not enabled in the firmware.");
        return;
    }

    pause_data_logging();
    RTE_ENABLE_SINGLE_SHOT_MODE;

    int rez = port_write_memory((const unsigned char*)&rtedbg_header.rte_cfg, RTE_CFG_WORD_ADDRESS, 4U);

    if (rez != RTE_OK)
    {
        return;
    }

    if (reset_circular_buffer() != RTE_OK)
    {
        return;
    }

    if (set_or_restore_message_filter() != RTE_OK)
    {
        return;
    }

    printf("\nSingle shot logging mode enabled and restarted.");
}


/***
 * @brief Switch to post mortem data logging mode.
 */

static void switch_to_post_mortem_logging(void)
{
    if (load_rtedbg_structure_header() != RTE_OK)
    {
        return;
    }

    pause_data_logging();

    if (RTE_SINGLE_SHOT_WAS_ACTIVE)
    {
        RTE_DISABLE_SINGLE_SHOT_MODE;
        int rez = port_write_memory((const unsigned char*)&rtedbg_header.rte_cfg, RTE_CFG_WORD_ADDRESS, 4U);

        if (rez != RTE_OK)
        {
            return;
        }
    }

    if (reset_circular_buffer() != RTE_OK)
    {
        return;
    }

    int ret = erase_buffer_index();     // Restart logging at the start of the circular buffer

    if (ret != RTE_OK)
    {
        printf("The RTEdbg buffer index may not be reset properly.");
    }

    ret = set_or_restore_message_filter();

    if (ret != RTE_OK)
    {
        printf("The message filter value may not be restored properly.");
    }


    if (!RTE_SINGLE_SHOT_WAS_ACTIVE)
    {
        printf("\nPost-mortem mode restarted.");
    }
    else
    {
        printf("\nPost-mortem logging mode enabled and restarted.");
    }
}


/***
 * @brief Print information from the g_rtedbg header structure.
 */

static void print_rtedbg_header_info(void)
{
    printf("\nCircular buffer size: %u words, last index: %u",
        rtedbg_header.buffer_size,
        rtedbg_header.last_index
    );

    printf(", timestamp frequency: %g MHz",
        (double)rtedbg_header.timestamp_frequency / 1e6 / (double)(uint64_t)(1ULL << RTE_TIMESTAMP_SHIFT)
    );

    printf(", long timestamps %s", RTE_USE_LONG_TIMESTAMP ? "enabled" : "disabled");

    if (RTE_SINGLE_SHOT_LOGGING_ENABLED && RTE_SINGLE_SHOT_WAS_ACTIVE)
    {
        printf(", single shot mode");
    }
    else
    {
        printf(", post-mortem mode");
    }

    if (!RTE_MSG_FILTERING_ENABLED)
    {
        printf("\nMessage filtering disabled in the firmware.");
    }
    else
    {
        print_filter_info();
    }
}


/***
 * @brief Set new message filter value.
 *         If the Enter key is pressed without a new value, the old value is retained.
 * 
 * @param filter_value  New filter value, NULL - enter new value manually.
 */

void set_new_filter_value(const char* filter_value)
{
    if (!RTE_MSG_FILTERING_ENABLED)
    {
        printf("\nMessage filtering disabled in the firmware.");
        return;
    }

    unsigned new_filter = 0;
    int no_entered = 0;

    if (filter_value == NULL)
    {
        printf("\nEnter new filter value -> -1=ALL (0x%X): ", parameters.filter);
        char number[50];
        fgets(number, sizeof(number) - 1, stdin);
        no_entered = sscanf_s(number, "%x", &new_filter);
    }
    else
    {
        no_entered = sscanf_s(filter_value, "%x", &new_filter);
    }

    if (no_entered == 1)
    {
        parameters.filter = new_filter;
    }

    parameters.set_filter = true;
        // Always set the embedded system filter even if the value has not been changed

    if (set_or_restore_message_filter() == RTE_OK)
    {
        printf("\nMessage filter set to 0x%X", parameters.filter);
    }
}


/***
 * @brief Execute the memory read benchmark.
 * 
 * The measurement is performed for a longer time (20 s) to show the effects of
 * non-real-time Windows scheduling.
 * The results are first written to a data field.
 * Full results are written to the speed_test.csv file and summary to the console.
 */

static void benchmark_data_transfer(void)
{
    double max_time = 0;
    double min_time = 9e99;
    double time_sum = 0;

    printf("\n\nMeasuring the read memory times...\nWait max. 20 seconds for the benchmark to complete.");

    if (!parameters.debug_mode)
    {
        enable_logging(false);    // Disable communication logging to speed up the data transfer
    }

    if (load_rtedbg_structure_header() != RTE_OK)
    {
        enable_logging(true);
        return;
    }

    double* time_used = (double*)malloc(BENCHMARK_REPEAT_COUNT * sizeof(double));

    if (time_used == NULL)
    {
        log_string("\nMemory allocation failed\n", NULL);
        enable_logging(true);
        return;
    }

    clock_t benchmark_start = clock_ms();
    size_t measurements;

    for (measurements = 0; measurements < BENCHMARK_REPEAT_COUNT;)
    {
        LARGE_INTEGER start_time;
        start_timer(&start_time);

        int rez = read_memory_block(
            (unsigned char*)p_rtedbg_structure,
            parameters.start_address,
            parameters.size
            );

        double time = time_elapsed(&start_time);
        time_used[measurements] = time;
        time_sum += time;

        if (rez != RTE_OK)
        {
            printf("\nBenchmark terminated prematurely - problem with reading from embedded system.");
            break;
        }

        measurements++;

        if (kbhit())
        {
            printf("\nBenchmark terminated with a keystroke.\n");
            break;
        }

        if ((clock_ms() - benchmark_start) > MAX_BENCHMARK_TIME_MS)
        {
            break;
        }

        if (time < min_time)
        {
            min_time = time;
        }

        if (time > max_time)
        {
            max_time = time;
        }
    }

    if (measurements > 1)
    {
        double min_speed = (double)parameters.size / max_time;
        double avg_speed = (double)parameters.size * (double)measurements / time_sum;

        FILE* report;
        int rez = fopen_s(&report, "speed_test.csv", "w");

        if (rez != 0)
        {
            char error_text[256];
#ifdef _WIN32
            (void)_strerror_s(error_text, sizeof(error_text), NULL);
#else
            strerror_s(error_text, sizeof(error_text), errno);
#endif
            printf("\nCannot create file 'speed_test.csv' - error: %s.\n", error_text);
        }
        else
        {
            fprintf(report, "Count;Time [ms];Data transfer speed [kB/s]\n");

            for (unsigned i = 0; i < measurements; i++)
            {
                fprintf(report, "%4u;%.1f;%.1f\n",
                    i + 1, time_used[i], (double)parameters.size / time_used[i]);
            }

            fprintf(report,
                "\nMinimal time %.1f ms, maximal time %.1f ms, block size %u bytes."
                "\nMinimal speed %.1f kB/s, average speed: %.1f kB/s.\n",
                min_time, max_time, parameters.size,
                min_speed, avg_speed
            );
            fclose(report);
        }

        printf(
            "\nMinimal time %.1f ms, maximal %.1f ms, block size %u bytes."
            "\nMinimal speed %.1f kB/s, average speed: %.1f kB/s."
            "\nSee the 'speed_test.csv' for details.\n",
            min_time, max_time, parameters.size,
            min_speed, avg_speed
        );
    }

    enable_logging(true);
    free(time_used);
}


/***
 * @brief  Display the status of logging in the embedded system.
 * 
 * @param start_time  Time when the persistent_connection() function started or
 *                    when the logging status was last displayed.
 */

static void display_logging_state(clock_t* start_time)
{
    clock_t current_time = clock_ms();

    if ((current_time - *start_time) < 350)
    {
        sleep_ms(50);
        return;
    }

    if (!parameters.debug_mode)
    {
        enable_logging(false);
    }

    port_handle_unexpected_messages();

    *start_time = current_time;
    int rez = load_rtedbg_structure_header();
    enable_logging(true);

    unsigned size = rtedbg_header.buffer_size - 4U;
    unsigned buffer_usage = (unsigned)((100U * rtedbg_header.last_index + size / 2U) / size);

    if (buffer_usage > 100)
    {
        buffer_usage = 100;
    }

    static bool cannot_get_data = false;

    if (rez == RTE_OK)
    {
        if (RTE_SINGLE_SHOT_WAS_ACTIVE && RTE_SINGLE_SHOT_LOGGING_ENABLED)
        {
            printf("\rIndex:%6d, filter: 0x%08X, %u%% used          ",
                rtedbg_header.last_index, rtedbg_header.filter, buffer_usage);
        }
        else
        {
            printf("\rIndex:%6d, filter: 0x%08X                     ",
                rtedbg_header.last_index, rtedbg_header.filter);
        }

        if (cannot_get_data)
        {
            // Overwrite the error message
            printf("                                      ");
        }

        cannot_get_data = false;
    }
    else
    {
        cannot_get_data = true;
        printf("\rCannot read data from the embedded system: %s            ",
            port_get_error_text());
    }
}


/***
 * @brief  Restart the file defined with the -start=command_file argument.
 */

static void repeat_start_command_file(void)
{
    if (parameters.start_cmd_file == NULL)
    {
        printf("\nCommand file not defined with the -start=command_file argument.");
    }
    else
    {
        (void)execute_commands_from_file(parameters.start_cmd_file);
    }
}



/***
 * @brief Keep connection to the server or COM port to enable multiple data transfers.
 *
 * @return RTE_OK    - no error
 *         RTE_ERROR - error
 */

static int persistent_connection(void)
{
    int rez = 0;
    clock_t start_time = clock_ms();
        // The time the function was started or the last time the loging status was displayed.

    printf("\nPress the '?' key for a list of available commands.\n");

    for (;;)
    {
        if (!kbhit())
        {
            display_logging_state(&start_time);
            continue;
        }

        int key = getch();

        if ((key == 0xE0) || (key == 0))    // Function key?
        {
            (void)getch();
            key = '\xFF';                   // Unknown command
        }

        switch(toupper(key))
        {
        case '?':
            show_help();
            break;

        case 'H':
            load_and_display_rtedbg_structure_header();
            break;

        case 'B':
            benchmark_data_transfer();
            break;

        case 'S':
            switch_to_single_shot_logging();
            break;

        case 'P':
            switch_to_post_mortem_logging();
            break;

        case 'F':
            set_new_filter_value(NULL);     // Enter new filter value
            break;

        case 'L':
            disable_enable_logging_to_file();
            break;

        case 'R':
            port_reconnect();
            break;

        case '0':
            repeat_start_command_file();
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            execute_commands_from_file_x((char)key);
            break;

        case ' ':
            rez = single_data_transfer();

            if ((rez != RTE_OK) && logging_to_file())
            {
                printf("\nError - check the log file for details.\n");
            }

            if (!logging_to_file())
            {
                printf("\n");
            }
            break;

        case '\x1B':
            printf("\n\nPress the 'Y' button to exit the program.");

            if (toupper(getch()) == 'Y')
            {
                return RTE_OK;
            }
            break;

        default:
            printf("\nUnknown command - Press the '?' key for a list of available commands.");
            break;
        }

        port_display_errors("\nCould not execute command: ");
    }
}


/***
 * @brief Execute commands from a ?.cmd file.
 *
 * @param name_start  This character replaces the '?' in the cmd file name.
 */

static void execute_commands_from_file_x(char name_start)
{
    char cmd_file_name[16];
    cmd_file_name[0] = name_start;
#ifdef _WIN32
    strcpy_s(&cmd_file_name[1], sizeof(cmd_file_name) - 1, ".cmd");
#else
    strcpy(&cmd_file_name[1], ".cmd");
#endif
    (void)execute_commands_from_file(cmd_file_name);
}


/***
 * @brief Get the g_rtedbg structure header from the embedded system.
 *
 * @return RTE_OK    - no error
 *         RTE_ERROR - data not received
 */

static int load_rtedbg_structure_header(void)
{
    int res = port_read_memory(
        (unsigned char *)&rtedbg_header, parameters.start_address, sizeof(rtedbg_header));

    if (res != RTE_OK)
    {
        return RTE_ERROR;
    }

    unsigned new_size = rtedbg_header.buffer_size * 4U + sizeof(rtedbg_header_t);

    if ((parameters.size == 0U)             // Automatically obtain the size of the structure
        || (new_size != parameters.size))   // Size changed
    {
        parameters.size = new_size;

        if (parameters.size < MIN_BUFFER_SIZE)
        {
            log_data(
                "\nThe buffer size specified in the g_rtedbg structure header is too small (%llu)",
                (long long)parameters.size);
             log_data(
                " < %llu).\n"
                "Check that the correct data structure address is passed as a parameter and that the rte_init() function has already been called.",
                (long long)MIN_BUFFER_SIZE);
            return RTE_ERROR;
        }

        if (parameters.size > MAX_BUFFER_SIZE)
        {
            log_data(
                "\nThe buffer size specified in the g_rtedbg structure header is too large (%llu)",
                (long long)parameters.size);
            log_data(
                " > %llu).\n"
                "Check that the correct data structure address is passed as a parameter and that the rte_init() function has already been called.",
                (long long)MAX_BUFFER_SIZE);
            return RTE_ERROR;
        }

        if (p_rtedbg_structure != NULL)
        {
            // The size has changed, release the buffer to allocate a new one.
            log_data("\nLog data structure changed to: %llu", new_size);
            free(p_rtedbg_structure);
            p_rtedbg_structure = NULL;
        }
    }

    if (!allocate_memory_for_g_rtedbg_structure())
    {
        return RTE_ERROR;
    }

    return RTE_OK;
}


/***
 * @brief Set the message filter to a new value (if defined as command line argument) or
 *        restore the old version. The filter_copy value is used if the filter is zero and
 *        the firmware can disable message filtering.
 *
 * @return RTE_OK    - no error
 *         RTE_ERROR - could not restore filter
 */

static int set_or_restore_message_filter(void)
{
    uint32_t old_filter = old_msg_filter;

    if ((old_filter == 0) && RTE_FILTER_OFF_ENABLED)
    {
        old_filter = rtedbg_header.filter_copy;
    }

    if (parameters.set_filter)
    {
        old_filter = parameters.filter;     // User defined filter value (command line argument)
    }

    return port_write_memory((const unsigned char *)&old_filter, MESSAGE_FILTER_ADDRESS, 4U);
}


/***
 * @brief Read the complete g_rtedbg structure from the embedded system and write it to a file.
 * 
 * @return RTE_OK    - no error
 *         RTE_ERROR - data not received or file operation failed
 */

static int save_rtedbg_structure(void)
{
    if (p_rtedbg_structure == NULL)
    {
        return RTE_ERROR;
    }

    delay_before_data_transfer();
    int err = read_memory_block(
        (unsigned char *)p_rtedbg_structure,
        parameters.start_address,
        parameters.size);

    if (err != RTE_OK)
    {
        return RTE_ERROR;
    }

    FILE * bin_file;
    errno_t rez = fopen_s(&bin_file, parameters.bin_file_name, "wb");

    printf("\nWriting data to a file");

    if ((rez != 0) && (errno == EACCES))
    {
        // Try again. The file may be temporarily locked.
        for (size_t i = 0; i < 9; i++)
        {
            putchar('.');
            sleep_ms(100);
            rez = fopen_s(&bin_file, parameters.bin_file_name, "wb");

            if (rez == 0)
            {
                break;
            }
        }
    }

    if (rez != 0)
    {
        printf("\n************************************************************");
        log_string("\nCould not create file \"%s\"", parameters.bin_file_name);
        char err_string[256];
        (void)strerror_s(err_string, sizeof(err_string), errno);
        log_string(": %s", err_string);

        if (logging_to_file())
        {
            printf("\nCould not create file \"%s\"", parameters.bin_file_name);
            printf(": %s", err_string);
        }

        printf("\n************************************************************\n");
        return RTE_ERROR;
    }

    // Restore the old message filter (as it was before logging was disabled)
    p_rtedbg_structure[1] = old_msg_filter;
    size_t written = fwrite(p_rtedbg_structure, 1U, parameters.size, bin_file);

    if (written != parameters.size)
    {
        log_string("\nCould not write to the file: %s.", parameters.bin_file_name);
        char error_text[256];
#ifdef _WIN32
        (void)_strerror_s(error_text, sizeof(error_text), NULL);
#else
        strerror_s(error_text, sizeof(error_text), errno);
#endif
        log_string(" Error: %s", error_text);
        (void)fclose(bin_file);

        if (logging_to_file())
        {
            printf("\nCould not write to the file: %s.", parameters.bin_file_name);
            printf(" Error: %s", error_text);
        }

        return RTE_ERROR;
    }

    (void)fclose(bin_file);
    return RTE_OK;
}


/***
 * @brief Read a block of memory from the embedded system.
 *
 * @param  buffer       buffer allocated for the read block
 * @param  address      start address of embedded system memory
 * @param  block_size   size in bytes
 *
 * @return RTE_OK    - no error
 *         RTE_ERROR - data not received
 */

static int read_memory_block(unsigned char * buffer, uint32_t address, uint32_t block_size)
{
    LARGE_INTEGER start_time;
    start_timer(&start_time);
    int res = port_read_memory(buffer, address, block_size);

    if (res != RTE_OK)
    {
        return RTE_ERROR;
    }

    long long speed =
        (long long)((double)block_size / time_elapsed(&start_time));

    if (speed > 20U)
    {
        log_data(", %llu kB/s. ", speed);
    }
    else
    {
        speed = (long long)((double)(block_size * 1000U) / time_elapsed(&start_time));
        log_data(", %llu B/s. ", speed);
    }

    return RTE_OK;
}


/***
 * @brief Pause data logging by erasing the message filter variable.
 *
 * @return RTE_OK    - no error
 *         RTE_ERROR - data not written
 */

static int pause_data_logging(void)
{
    return port_write_memory(
        (const unsigned char *)"\x00\x00\x00\x00", MESSAGE_FILTER_ADDRESS, 4U);
}


/***
 * @brief Erase the circular buffer index.
 * 
 * @return RTE_OK    - no error
 *         RTE_ERROR - buffer index not erased (data not written)
 */

static int erase_buffer_index(void)
{
    return port_write_memory(
        (const unsigned char *)"\x00\x00\x00\x00", parameters.start_address, 4U);
}


/***
 * @brief Check if the single shot logging was enabled and is active.
 * 
 * @return true  - single shot enabled and active
 *         false - single shot disabled (compile time) or enabled but not active
 */

static bool single_shot_active(void)
{
    if ((RTE_SINGLE_SHOT_WAS_ACTIVE != 0) && (RTE_SINGLE_SHOT_LOGGING_ENABLED != 0))
    {
        return true;
    }

    return false;
}


/***
 * @brief Reset the complete contents of the circular buffer to 0xFFFFFFFF if enabled
 *        or reset just the buffer index if not enabled and single shot logging
 *        was active.
 * 
 * @return RTE_OK    - no error
 *         RTE_ERROR - buffer not reset
 */

static int reset_circular_buffer(void)
{
    int rez = RTE_OK;

    if (parameters.clear_buffer)
    {
        unsigned circular_buffer_size = parameters.size - sizeof(rtedbg_header);
        unsigned char* circular_buffer = (unsigned char*)malloc(circular_buffer_size);

        if (circular_buffer == NULL)
        {
            log_string("\nCould not allocate memory buffer.", NULL);
            return RTE_ERROR;
        }

        memset(circular_buffer, 0xFFU, circular_buffer_size);

        LARGE_INTEGER start_time;
        start_timer(&start_time);
        printf("\nClearing the circular buffer...");

        rez = port_write_memory(
            circular_buffer,
            parameters.start_address + sizeof(rtedbg_header_t),
            circular_buffer_size);
        free(circular_buffer);

        if (rez != RTE_OK)
        {
            return RTE_ERROR;
        }

        unsigned block_size = parameters.size - sizeof(rtedbg_header);
        long long speed =
            (long long)((double)block_size / time_elapsed(&start_time));

        if (speed > 20U)
        {
            log_data(", %llu kB/s. ", speed);
        }
        else
        {
            speed = (long long)((double)(block_size * 1000U) / time_elapsed(&start_time));
            log_data(", %llu B/s. ", speed);
        }
    }

    if (parameters.clear_buffer || single_shot_active())
    {
        rez = erase_buffer_index();   // Restart logging at the start of the circular buffer
    }

    return rez;
}


/***
 * @brief Check that the information in the g_rtedbg header is correct. 
 * The tester may be using the wrong g_rtedbg structure address,
 * or the structure may not be initialized.
 *
 * @return RTE_OK    - no error
 *         RTE_ERROR - incorrect header information
 */

static int check_header_info(void)
{
    if ((sizeof(rtedbg_header_t) != RTE_HEADER_SIZE) 
        || (RTE_CFG_RESERVED_BITS != 0)
        || (RTE_CFG_RESERVED2 != 0)
        )
    {
        log_string(
            "\nError in the g_rtedbg structure header (incorrect header size / reserved bits).\n"
            "Check that the correct data structure address is passed as a parameter"
            " and that the rte_init() function has already been called.",
            NULL
        );
        return RTE_ERROR;
    }

    return RTE_OK;
}


/***
 * @brief Initialize the data structure header and set the circular buffer to 0xFFFFFFFF.
 *        This function is intended for projects where rte_init() is not called - for
 *        resource constrained systems.
 * 
 * @param cfg_word              g_rtedbg configuration word
 * @param timestamp_frequency   timestamp timer clock frequency
 */

void initialize_data_logging_structure(unsigned cfg_word, unsigned timestamp_frequency)
{
    if (timestamp_frequency == 0)
    {
        log_string("- the timestamp frequency must not be zero", NULL);
        return;
    }

    if (parameters.size == 0)
    {
        log_string("- the size command line argument must not be zero", NULL);
        return;
    }

    rtedbg_header_t rtedbg;
    rtedbg.last_index = 0;
    rtedbg.filter = 0;
    rtedbg.filter_copy = parameters.filter;
    rtedbg.buffer_size = (parameters.size - sizeof(rtedbg_header_t)) / 4U;
    rtedbg.timestamp_frequency = timestamp_frequency;
    rtedbg.rte_cfg = cfg_word;

    // Disable logging during the g_rtedbg structure initialization
    if (pause_data_logging() != RTE_OK)
    {
        return;
    }

    // Write the header of the g_rtedbg structure to the embedded system
    int rez = port_write_memory((const unsigned char*)&rtedbg, parameters.start_address, sizeof(rtedbg));
    if (rez != RTE_OK)
    {
        return;
    }

    if (reset_circular_buffer() != RTE_OK)
    {
        return;
    }

    if (parameters.filter != 0)
    {
        // Enable logging
        rez = port_write_memory((const unsigned char*)&parameters.filter, MESSAGE_FILTER_ADDRESS, 4U);

        if (rez != RTE_OK)
        {
            return;
        }
    }

    log_string("\nThe g_rtedbg data logging structure has been initialized. ", NULL);
}


/***
 * @brief Execute a delay if defined with a command line parameter.
 *        The delay enables low priority tasks to finish writing to the circular buffer.
 */

static void delay_before_data_transfer(void)
{
    if (parameters.delay > 0)
    {
        log_data("\nDelay %llu ms", (long long)parameters.delay);
        sleep_ms(parameters.delay);
            // Wait for low priority tasks to finish writing to the circular buffer
    }
}


/***
 * @brief Allocate memory for the g_rtedbg logging structure if the size is known.
 * 
 * @return true if memory was successfully allocated or already allocated
 *         false if memory could not be allocated
 */

static bool allocate_memory_for_g_rtedbg_structure(void)
{
    if (parameters.size == 0)
    {
        return false;
    }

    if (p_rtedbg_structure != NULL)
    {
        return true;    // Memory already allocated
    }

    p_rtedbg_structure = (unsigned*)malloc(parameters.size);

    if (p_rtedbg_structure == NULL)
    {
        log_string("\nCould not allocate memory buffer.", NULL);
        return false;
    }

    return true;        // Memory allocation successful
}


/***
 * @brief How much time [ms] has passed since the CRT initialization during process start.
 * 
 * @return Time elapsed in miliseconds.
 */

long clock_ms(void)
{
    clock_t time = clock();

    if (time == -1)
    {
        return -1;
    }

    double time_ms = 0.5 + (double)time / (double)CLOCKS_PER_SEC * 1000.;

    if (time_ms > (double)INT32_MAX)
    {
        return -1;
    }

    return (long)time_ms;
}


/***
 * @brief Execute internal command - the following ones are available:
 *    #delay xxx - delay xxx ms
 *    #init config_word timestamp_frequency
 *    #filter value - set a new filter value
 *    #echo text - echo the text
 *
 * @param cmd_text  Pointer to the command text
 */

static void internal_command(const char* cmd_text)
{
    if (strncmp(cmd_text, "##", 2) == 0)
    {
        return;     // Ignore comments
    }

    if (strncmp(cmd_text, "#echo ", 6) != 0)
    {
        if (logging_to_file())
        {
            printf("\n   \"%s\" ", cmd_text);
        }

        log_string("\n   \"%s\" ", cmd_text);
    }

    if (strncmp(cmd_text, "#delay ", 7) == 0)
    {
        unsigned delay_ms = 0;
        int fields = sscanf_s(&cmd_text[7], "%u", &delay_ms);

        if ((fields == 1) && (delay_ms > 0))
        {
            if (logging_to_file())
            {
                printf("\ndelay %u ms", delay_ms);
            }
            sleep_ms(delay_ms);
            port_flush();
        }
    }
    else if (strncmp(cmd_text, "#init ", 6) == 0)
    {
        unsigned cfg_word = 0;
        unsigned timestamp_frequency = 0;

        if (sscanf_s(&cmd_text[6], "%x %u", &cfg_word, &timestamp_frequency) == 2)
        {
            printf("\nLogging data structure initialization");
            initialize_data_logging_structure(cfg_word, timestamp_frequency);
        }
        else
        {
            log_string("- #init command must have two parameters: config word (hex) and timestamp frequency (decimal value) ", NULL);
        }
    }
    else if (strncmp(cmd_text, "#filter ", 8) == 0)
    {
        set_new_filter_value(&cmd_text[8]);
    }
    else if (strncmp(cmd_text, "#echo ", 6) == 0)
    {
        printf("\n   %s", &cmd_text[6]);
    }
    else
    {
        log_string("- unknown command", NULL);
    }
}


/***
 * @brief Execute commands or send them over a GDB or COM port.
 *
 * @param cmd_file  File with commands
 *
 * @return RTE_OK    - no error
 *         RTE_ERROR - error
 */

int execute_commands_from_file(const char* cmd_file)
{
    if (cmd_file == NULL)
    {
        return RTE_OK;
    }

    port_handle_unexpected_messages();  // Discard potentially unhandled data (e.g. after reset)

    if (logging_to_file())
    {
        printf("\nExecute command file: \"%s\" ...", cmd_file);
    }

    log_string("\nExecute command file: \"%s\" ...", cmd_file);

    FILE* commands;
    errno_t err = fopen_s(&commands, cmd_file, "r");

    if ((err != 0) || (commands == NULL))
    {
        char err_string[256];
        (void)strerror_s(err_string, sizeof(err_string), errno);
        log_string("\nCould not open command file - error: %s \n", err_string);

        if (logging_to_file())
        {
            printf("\nCould not open command file - error: %s \n", err_string);
        }

        return RTE_ERROR;
    }

    for (;;)
    {
        char cmd_text[512];
        char* rez = fgets(cmd_text, sizeof(cmd_text), commands);

        if (rez == NULL)
        {
            if (!feof(commands))
            {
                (void)strerror_s(cmd_text, sizeof(cmd_text), errno);
                log_string(": can't read from file - error: %s\n", cmd_text);

                if (logging_to_file())
                {
                    printf(": can't read from file - error: %s\n", cmd_text);
                }
            }
            break;
        }

        rez = strchr(cmd_text, '\n');

        if (rez != NULL)
        {
            *rez = '\0';        // Strip the newline at the end of line
        }

        if (strlen(cmd_text) > 0)
        {
            if (*cmd_text == '#')
            {
                internal_command(cmd_text);
            }
            else
            {
                if (port_execute_command(cmd_text) != RTE_OK)
                {
                    break;
                }
            }
        }
    }

    (void)fclose(commands);
    printf("\n");

    return RTE_OK;
}

/*==== End of file ====*/
