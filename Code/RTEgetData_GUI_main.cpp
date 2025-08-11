/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file      RTEgetData_GUI_main.cpp
 * @brief     Main entry point for RTEgetData GUI application
 * @author    B. Premzel
 */

#include "RTEgetData_GUI.h"
#include "cmd_line.h"
#include "rtedbg.h"
#include <iostream>
#include <chrono>
#include <cstring>

// Global variables required by the core RTEgetData modules
parameters_t parameters;             // Command line parameters
err_code_t last_error = ERR_NO_ERROR;  // Last error code

// Timing function required by core modules
long clock_ms(void) {
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

// Main entry point for GUI application
int main() {
    // Initialize global variables with reasonable defaults
    memset(&parameters, 0, sizeof(parameters));
    
    // Set some default parameters
    parameters.gdb_port = 3333;
    parameters.active_interface = GDB_PORT;
    parameters.com_port.baudrate = 115200;
    parameters.com_port.parity = 0; // NOPARITY
    parameters.com_port.stop_bits = 1;
    parameters.com_port.recv_start_timeout = 50;
    
    RTEgetDataGUI app;
    
    if (!app.Initialize()) {
        std::cerr << "Failed to initialize RTEgetData GUI" << std::endl;
        return 1;
    }
    
    app.Run();
    app.Shutdown();
    
    return 0;
}