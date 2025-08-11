/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file      RTEgetData_GUI.cpp
 * @brief     GUI interface for RTEgetData using Dear ImGui
 * @author    B. Premzel
 */

#include "RTEgetData_GUI.h"
#include "RTEgetData.h"
#include "cmd_line.h"
#include "bridge.h"
#include "logger.h"
#include "gdb_lib.h"
#include "com_lib.h"

// ImGui includes
#include "imgui.h"
#ifdef GUI_USE_SDL2
    #include "imgui_impl_sdl2.h"
    #include "imgui_impl_opengl3.h"
    #include <SDL.h>
    #include <SDL_opengl.h>
#else
    #include "imgui_impl_glfw.h"
    #include "imgui_impl_opengl3.h"
    #include <GLFW/glfw3.h>
    #ifdef _WIN32
        #include <windows.h>
        #include <GL/gl.h>
    #else
        #include <GL/gl.h>
    #endif
#endif
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <vector>
#include <cstring>
#include <fstream>
#include <regex>
#include <algorithm>
#ifdef _WIN32
    #include <windows.h>
    #include <setupapi.h>
    #include <devguid.h>
    #include <cfgmgr32.h>
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif

// Global GUI instance for C-style callbacks
static RTEgetDataGUI* g_gui_instance = nullptr;

#ifndef GUI_USE_SDL2
// GLFW error callback
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}
#endif

RTEgetDataGUI::RTEgetDataGUI() {
    g_gui_instance = this;
    m_currentDirectory = std::filesystem::current_path();
}

RTEgetDataGUI::~RTEgetDataGUI() {
    Shutdown();
    g_gui_instance = nullptr;
}

bool RTEgetDataGUI::Initialize() {
#ifdef GUI_USE_SDL2
    // Setup SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window with graphics context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow("RTEgetData - GUI Interface", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    if (!m_window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init(glsl_version);

#else
    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window
    m_window = glfwCreateWindow(1280, 720, "RTEgetData - GUI Interface", NULL, NULL);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
#endif

    // Load settings
    LoadSettings();

    // Initialize COM port list
    RefreshComPorts();

    // Start background thread
    m_backgroundRunning = true;
    m_backgroundThread = std::thread(&RTEgetDataGUI::BackgroundWorker, this);

    m_initialized = true;
    AddLogMessage("RTEgetData GUI initialized successfully");

    return true;
}

void RTEgetDataGUI::Run() {
    if (!m_initialized) {
        return;
    }

#ifdef GUI_USE_SDL2
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Enable docking (if available)
#ifdef IMGUI_HAS_DOCK
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif

        // Show single integrated main window
        ShowMainWindow();

        // Conditional windows
        if (m_showDemo) {
            ImGui::ShowDemoWindow(&m_showDemo);
        }
        if (m_showAbout) {
            ShowAboutWindow();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_window);
    }
#else
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Enable docking (if available)
#ifdef IMGUI_HAS_DOCK
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif

        // Show single integrated main window
        ShowMainWindow();

        // Conditional windows
        if (m_showDemo) {
            ImGui::ShowDemoWindow(&m_showDemo);
        }
        if (m_showAbout) {
            ShowAboutWindow();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
#endif
}

void RTEgetDataGUI::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Stop background thread
    m_shouldStop = true;
    if (m_backgroundThread.joinable()) {
        m_backgroundThread.join();
    }

    // Save settings
    SaveSettings();

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
#ifdef GUI_USE_SDL2
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL2
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
    SDL_Quit();
#else
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup GLFW
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
#endif

    m_initialized = false;
    AddLogMessage("RTEgetData GUI shutdown");
}

void RTEgetDataGUI::ShowMainWindow() {
    // Create main window using most of the screen
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("RTEgetData", nullptr, ImGuiWindowFlags_MenuBar);
    
    // Create main layout with splitter
    ImGui::Columns(2, "MainLayout", true);
    
    // Left column - Control Panel
    ImGui::BeginChild("ControlPanel", ImVec2(0, 0), true);
    ImGui::Text("RTEgetData Control Panel");

    // Connection Type Selection
    ImGui::Text("Connection Type:");
    ImGui::SameLine();
    if (ImGui::RadioButton("GDB Server", m_connectionType == ConnectionType::GDB_SERVER)) {
        m_connectionType = ConnectionType::GDB_SERVER;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Connect via GDB server over TCP/IP network connection.\n"
                         "Use this for:\n"
                         "• Debug probes: J-Link, ST-Link, OpenOCD\n"
                         "• Transferring RTEdbg log data via debug probe\n"
                         "• Parallel operation with IDE debugger (if supported)");
    }
    
    ImGui::SameLine();
    if (ImGui::RadioButton("COM Port", m_connectionType == ConnectionType::COM_PORT)) {
        m_connectionType = ConnectionType::COM_PORT;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Connect directly via serial port (UART/USB).\n"
                         "Use this for:\n"
                         "• Direct USB or serial cable connection\n"
                         "• Arduino or similar microcontroller boards\n"
                         "• When no debug probe is available");
    }

    ImGui::Separator();

    // Connection Settings
    if (m_connectionType == ConnectionType::GDB_SERVER) {
        ImGui::Text("GDB Server Settings:");
        
        ImGui::InputText("IP Address", m_settings.gdb_ip, sizeof(m_settings.gdb_ip));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("IP address of the GDB server.\n"
                             "• 127.0.0.1 or localhost - Local server\n"
                             "• 192.168.x.x - Network server\n"
                             "• Usually same as your debug probe's IP");
        }
        
        ImGui::InputInt("Port", &m_settings.gdb_port);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("TCP port of the GDB server.\n"
                             "Default ports:\n"
                             "• 2331 - J-Link GDB server\n"
                             "• 61234 - ST-LINK GDB server\n"
                             "• 3333 - OpenOCD GDB server\n"
                             "Check your debugger documentation for the correct port.");
        }
    } else {
        ImGui::Text("COM Port Settings:");
        
        // Refresh COM ports if needed
        if (m_comPortsNeedRefresh) {
            RefreshComPorts();
        }
        
        // COM port selection dropdown
        ImGui::Text("Port:");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Select the serial port connected to your device.\n"
                             "Linux: /dev/ttyUSB0, /dev/ttyACM0, etc.\n"
                             "Windows: COM1, COM2, etc.\n"
                             "Use 'Refresh' to update the list after connecting devices.");
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            RefreshComPorts();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Refresh the list of available COM ports.\n"
                             "Click this after plugging in or unplugging devices.");
        }
        
        if (m_availableComPorts.empty()) {
            ImGui::Text("No COM ports found");
            ImGui::InputText("Manual Entry", m_settings.com_port, sizeof(m_settings.com_port));
        } else {
            // Create dropdown with port descriptions
            std::vector<const char*> portItems;
            for (const auto& port : m_availableComPorts) {
                portItems.push_back(port.friendly_name.c_str());
            }
            
            if (ImGui::Combo("##ComPortCombo", &m_selectedComPortIndex, portItems.data(), portItems.size())) {
                // Update the settings when selection changes
                if (m_selectedComPortIndex >= 0 && m_selectedComPortIndex < static_cast<int>(m_availableComPorts.size())) {
                    strncpy(m_settings.com_port, m_availableComPorts[m_selectedComPortIndex].device.c_str(), sizeof(m_settings.com_port) - 1);
                    m_settings.com_port[sizeof(m_settings.com_port) - 1] = '\0';
                }
            }
            
            // Show selected port path
            if (m_selectedComPortIndex >= 0 && m_selectedComPortIndex < static_cast<int>(m_availableComPorts.size())) {
                ImGui::Text("Device: %s", m_availableComPorts[m_selectedComPortIndex].device.c_str());
            }
        }
        
        ImGui::InputInt("Baud Rate", &m_settings.com_baudrate);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Serial communication speed in bits per second.\n"
                             "Common rates:\n"
                             "• 9600 - Very slow, very reliable\n"
                             "• 115200 - Standard speed (recommended)\n"
                             "• 230400, 460800 - High speed\n"
                             "• 1000000+ - Very high speed\n"
                             "Must match your device's configuration!");
        }
        
        ImGui::Text("Parity:");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Error checking method for serial communication.\n"
                             "• None - No error checking (most common)\n"
                             "• Odd/Even - Adds parity bit for error detection\n"
                             "Must match your device settings!");
        }
        ImGui::SameLine();
        ImGui::RadioButton("None", &m_settings.com_parity, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Odd", &m_settings.com_parity, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Even", &m_settings.com_parity, 2);
        
        ImGui::InputInt("Stop Bits", &m_settings.com_stopbits);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Number of stop bits for serial communication.\n"
                             "• 1 - Standard (most common)\n"
                             "• 2 - Used for slower or noisy connections\n"
                             "Must match your device configuration!");
        }
        
        ImGui::InputInt("Timeout (ms)", &m_settings.com_timeout);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("How long to wait for device response (milliseconds).\n"
                             "• 50-100ms - Fast, reliable connections\n"
                             "• 500-1000ms - Slow or wireless connections\n"
                             "• 2000ms+ - Very slow connections\n"
                             "Increase if getting timeout errors.");
        }
        
        ImGui::Checkbox("Single Wire Mode", &m_settings.single_wire);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enable single-wire serial communication.\n"
                             "Used for:\n"
                             "• Half-duplex communication\n"
                             "• Systems with shared TX/RX line\n"
                             "• Some proprietary protocols\n"
                             "Leave unchecked for normal UART connections.");
        }
    }

    ImGui::Separator();

    // Transfer Settings
    ImGui::Text("Data Transfer Settings:");
    
    ImGui::InputText("Memory Address", m_transfer.address, sizeof(m_transfer.address));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Starting memory address to read from (hexadecimal).\n"
                         "Examples:\n"
                         "• 0x20000000 - Typical RAM start address\n"
                         "• 0x08000000 - Typical Flash start address\n"
                         "• 0x24000000 - RTEgetData buffer location\n"
                         "Use your linker map or debugger to find the correct address.");
    }
    
    ImGui::InputText("Size", m_transfer.size, sizeof(m_transfer.size));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Number of bytes to read (hexadecimal).\n"
                         "Examples:\n"
                         "• 0x100 - 256 bytes\n"
                         "• 0x1000 - 4KB\n"
                         "• 0x10000 - 64KB\n"
                         "Must not exceed available memory or buffer size!");
    }
    
    ImGui::InputText("Output File", m_transfer.output_file, sizeof(m_transfer.output_file));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("File to save the transferred data.\n"
                         "Examples:\n"
                         "• data.bin - Binary data file\n"
                         "• log.dat - Log data file\n"
                         "• /path/to/output.bin - Full path\n"
                         "File will be created or overwritten.");
    }
    
    ImGui::Checkbox("Clear Buffer", &m_transfer.clear_buffer);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear the embedded system's buffer after transfer.\n"
                         "• Checked - Prevents reading old data on next transfer\n"
                         "• Unchecked - Keeps data in buffer (for debugging)\n"
                         "Recommended for normal operation.");
    }
    
    ImGui::Checkbox("Persistent Mode", &m_transfer.persistent_mode);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Keep connection open for multiple transfers.\n"
                         "• Checked - Faster for multiple transfers\n"
                         "• Unchecked - Disconnect after each transfer\n"
                         "Use for repeated data collection.");
    }
    
    ImGui::InputInt("Delay (ms)", &m_transfer.delay_ms);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Delay before starting data transfer (milliseconds).\n"
                         "• 0 - No delay (fastest)\n"
                         "• 100-500ms - Allow system to stabilize\n"
                         "• 1000ms+ - For slow embedded systems\n"
                         "Increase if getting incomplete data.");
    }

    ImGui::Separator();

    // Action Buttons
    bool can_connect = ((m_operationState == OperationState::IDLE || m_operationState == OperationState::ERROR) && !m_isConnected);
    bool can_transfer = (m_isConnected && m_operationState != OperationState::TRANSFERRING);
    bool can_disconnect = m_isConnected;
    
    if (!can_connect) {
        ImGui::BeginDisabled();
    }
    
    const char* connect_label = (m_operationState == OperationState::ERROR) ? "Retry Connection" : "Connect";
    if (ImGui::Button(connect_label)) {
        StartBackgroundTask([this]() { 
            ConnectToTarget();
        });
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Establish connection to the target device.\n"
                         "• GDB Server: Connects via TCP/IP to debug server\n"
                         "• COM Port: Opens serial port connection\n"
                         "Must be connected before transferring data.");
    }
    
    if (!can_connect) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    
    if (!can_transfer) {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("Transfer Data")) {
        StartBackgroundTask([this]() { 
            TransferData();
        });
    }
    if (ImGui::IsItemHovered() && can_transfer) {
        ImGui::SetTooltip("Read data from the target device's memory.\n"
                         "• Reads from specified memory address\n"
                         "• Saves data to output file\n"
                         "• Shows progress during transfer\n"
                         "Must be connected first!");
    }
    
    if (!can_transfer) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    
    if (!can_disconnect) {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("Disconnect")) {
        StartBackgroundTask([this]() { 
            DisconnectFromTarget();
        });
    }
    if (ImGui::IsItemHovered() && can_disconnect) {
        ImGui::SetTooltip("Close connection to the target device.\n"
                         "• Frees the COM port or GDB connection\n"
                         "• Allows other tools to use the connection\n"
                         "• Recommended when finished transferring data");
    }
    
    if (!can_disconnect) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Clear Log")) {
        ClearLog();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear all log messages.\n"
                         "Useful for cleaning up the display before\n"
                         "starting a new operation.");
    }

    // Operation Status
    ImGui::Separator();
    ImGui::Text("Status: ");
    ImGui::SameLine();
    
    switch (m_operationState) {
        case OperationState::IDLE:
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Ready");
            break;
        case OperationState::CONNECTING:
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Connecting...");
            break;
        case OperationState::TRANSFERRING:
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Transferring...");
            break;
        case OperationState::COMPLETED:
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Completed");
            break;
        case OperationState::ERROR:
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error - Ready to retry");
            break;
    }

    if (!m_progress.current_operation.empty()) {
        ImGui::Text("Operation: %s", m_progress.current_operation.c_str());
        ImGui::ProgressBar(m_progress.progress, ImVec2(0.0f, 0.0f));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Progress of current operation.\n"
                             "Shows completion percentage for:\n"
                             "• Connection establishment\n"
                             "• Data transfer\n"
                             "• File writing operations");
        }
    }
    
    // Status bar at bottom of control panel
    ImGui::Separator();
    ImGui::Text("Ready");
    if (m_progress.bytes_transferred > 0) {
        ImGui::Text("%s / %s transferred", 
                   FormatFileSize(m_progress.bytes_transferred).c_str(),
                   FormatFileSize(m_progress.total_bytes).c_str());
    }
    
    // Demo and About buttons
    if (ImGui::Button("Show Demo")) {
        m_showDemo = !m_showDemo;
    }
    ImGui::SameLine();
    if (ImGui::Button("About")) {
        m_showAbout = !m_showAbout;
    }
    
    ImGui::EndChild(); // End ControlPanel
    
    // Right column - Log Panel
    ImGui::NextColumn();
    
    ImGui::BeginChild("LogPanel", ImVec2(0, 0), true);
    ImGui::Text("Log Messages");
    
    // Log controls
    if (ImGui::Button("Clear")) {
        ClearLog();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear all log messages from the display.");
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically scroll to show newest messages.\n"
                         "Disable if you want to read older messages\n"
                         "without them scrolling away.");
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Show timestamps", &m_showTimestamps);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show timestamp for each log message.\n"
                         "Useful for tracking timing of operations\n"
                         "and debugging connection issues.");
    }

    ImGui::Separator();

    // Log display
    ImGui::BeginChild("LogScrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    std::lock_guard<std::mutex> lock(m_logMutex);
    for (const auto& entry : m_logMessages) {
        ImVec4 color;
        switch (entry.level) {
            case 0: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break; // Info - white
            case 1: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; // Warning - yellow
            case 2: color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break; // Error - red
            default: color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); break; // Debug - gray
        }
        
        if (m_showTimestamps) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s]", entry.timestamp.c_str());
            ImGui::SameLine();
        }
        
        ImGui::TextColored(color, "%s", entry.message.c_str());
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild(); // End LogScrolling
    ImGui::EndChild(); // End LogPanel
    
    // Conditional popup windows
    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
    if (m_showAbout) {
        ShowAboutWindow();
    }

    ImGui::End(); // End main window
}


void RTEgetDataGUI::ShowAboutWindow() {
    ImGui::Begin("About RTEgetData", &m_showAbout);
    
    ImGui::Text("RTEgetData GUI v%s", RTEGETDATA_VERSION);
    ImGui::Text("Built on: %s", __DATE__);
    ImGui::Separator();
    
    ImGui::TextWrapped(
        "RTEgetData is a utility for transferring binary log data from embedded systems "
        "using either a GDB server or COM port connection."
    );
    
    ImGui::Spacing();
    ImGui::Text("Features:");
    ImGui::BulletText("GDB server communication (TCP/IP)");
    ImGui::BulletText("Serial port communication");
    ImGui::BulletText("Real-time progress monitoring");
    ImGui::BulletText("Cross-platform support (Windows/Linux)");
    
    ImGui::Spacing();
    if (ImGui::Button("Close")) {
        m_showAbout = false;
    }
    
    ImGui::End();
}


void RTEgetDataGUI::ConnectToTarget() {
    // Reset state for new connection attempt
    m_operationState = OperationState::CONNECTING;
    m_isConnected = false;
    m_progress.current_operation = "Connecting to target";
    m_progress.progress = 0.1f;
    m_progress.bytes_transferred = 0;
    m_progress.total_bytes = 0;
    
    AddLogMessage("Starting connection...");
    
    int result = RTE_ERROR;
    
    // Update global parameters with GUI settings
    if (m_connectionType == ConnectionType::GDB_SERVER) {
        AddLogMessage("Connecting to GDB server at " + 
                     std::string(m_settings.gdb_ip) + ":" + 
                     std::to_string(m_settings.gdb_port));
        
        parameters.active_interface = GDB_PORT;
        parameters.gdb_port = m_settings.gdb_port;
        parameters.ip_address = m_settings.gdb_ip;
        
        result = gdb_connect(m_settings.gdb_port);
        
    } else {
        parameters.active_interface = COM_PORT;
        
        // Use selected COM port from dropdown, or manual entry if no ports found
        std::string selectedPort = m_settings.com_port;
        if (!m_availableComPorts.empty() && 
            m_selectedComPortIndex >= 0 && 
            m_selectedComPortIndex < static_cast<int>(m_availableComPorts.size())) {
            selectedPort = m_availableComPorts[m_selectedComPortIndex].device;
        }
        
        AddLogMessage("Opening COM port: " + selectedPort);
        parameters.com_port.name = const_cast<char*>(selectedPort.c_str());
        parameters.com_port.baudrate = m_settings.com_baudrate;
        parameters.com_port.parity = m_settings.com_parity;
        parameters.com_port.stop_bits = m_settings.com_stopbits;
        parameters.com_port.recv_start_timeout = m_settings.com_timeout;
        parameters.com_port.single_wire_communication = m_settings.single_wire;
        
        result = com_open();
    }
    
    m_progress.progress = 0.8f;
    
    if (result == RTE_OK) {
        m_operationState = OperationState::COMPLETED;
        m_isConnected = true;
        AddLogMessage("Connection established successfully");
        m_progress.current_operation = "Connected";
        m_progress.progress = 1.0f;
    } else {
        m_operationState = OperationState::ERROR;
        std::string error_msg = "Connection failed";
        
        // Get specific error message
        if (m_connectionType == ConnectionType::GDB_SERVER) {
            const char* gdb_error = gdb_get_error_text();
            if (gdb_error && strlen(gdb_error) > 0) {
                error_msg += ": " + std::string(gdb_error);
            }
        } else {
            // Map common COM port errors to user-friendly messages
            switch (last_error) {
                case ERR_COM_CANNOT_OPEN_PORT:
                    error_msg += ": Cannot open COM port. Check port name and permissions.";
                    break;
                case ERR_COM_RECEIVE:
                    error_msg += ": Communication error. Check baud rate and connection.";
                    break;
                default:
                    error_msg += ": Error code " + std::to_string(last_error);
                    break;
            }
        }
        
        AddLogMessage(error_msg, 2); // Error level
        m_progress.current_operation = "Connection failed";
        
        // Clean up any partial connection state
        if (m_connectionType == ConnectionType::GDB_SERVER) {
            gdb_detach(); // Clean up any partial GDB connection
        } else {
            com_close(); // Clean up any partial COM connection
        }
        
        // Reset state to allow retry
        m_isConnected = false;
    }
}

void RTEgetDataGUI::TransferData() {
    m_operationState = OperationState::TRANSFERRING;
    m_progress.current_operation = "Transferring data";
    m_progress.progress = 0.0f;
    
    try {
        // Parse address and size
        unsigned int address = std::stoul(m_transfer.address, nullptr, 16);
        unsigned int size = std::stoul(m_transfer.size, nullptr, 16);
        
        AddLogMessage("Starting data transfer from address " + 
                     std::string(m_transfer.address) + 
                     ", size " + std::string(m_transfer.size));
        
        m_progress.total_bytes = size;
        m_progress.bytes_transferred = 0;
        
        // Allocate buffer for data
        std::vector<unsigned char> buffer(size);
        
        // Update global parameters with transfer settings  
        parameters.start_address = address;
        parameters.size = size;
        parameters.bin_file_name = m_transfer.output_file;
        
        // Perform the actual memory read
        int result = port_read_memory(buffer.data(), address, size);
        
        if (result == RTE_OK) {
            m_progress.progress = 0.8f;
            m_progress.bytes_transferred = size;
            
            // Write data to file
            FILE* file = fopen(m_transfer.output_file, "wb");
            if (file != nullptr) {
                size_t written = fwrite(buffer.data(), 1, size, file);
                fclose(file);
                
                if (written == size) {
                    m_operationState = OperationState::COMPLETED;
                    AddLogMessage("Data transfer completed successfully");
                    AddLogMessage("Data saved to: " + std::string(m_transfer.output_file) + 
                                 " (" + std::to_string(size) + " bytes)");
                    m_progress.current_operation = "Transfer complete";
                    m_progress.progress = 1.0f;
                } else {
                    m_operationState = OperationState::ERROR;
                    AddLogMessage("Error writing to file: Only " + std::to_string(written) + 
                                 " of " + std::to_string(size) + " bytes written", 2);
                }
            } else {
                m_operationState = OperationState::ERROR;
                AddLogMessage("Error: Cannot create output file: " + std::string(m_transfer.output_file), 2);
            }
        } else {
            m_operationState = OperationState::ERROR;
            std::string error_msg = "Memory read failed";
            
            // Get specific error message based on connection type
            if (m_connectionType == ConnectionType::GDB_SERVER) {
                const char* gdb_error = gdb_get_error_text();
                if (gdb_error && strlen(gdb_error) > 0) {
                    error_msg += ": " + std::string(gdb_error);
                }
            } else {
                switch (last_error) {
                    case ERR_RCV_TIMEOUT:
                        error_msg += ": Receive timeout. Check connection.";
                        break;
                    case ERR_COM_RECEIVE:
                        error_msg += ": Communication error during transfer.";
                        break;
                    case ERR_BAD_INPUT_DATA:
                        error_msg += ": Invalid memory address or size.";
                        break;
                    default:
                        error_msg += ": Error code " + std::to_string(last_error);
                        break;
                }
            }
            
            AddLogMessage(error_msg, 2);
            m_progress.current_operation = "Transfer failed";
            
            // Keep connection but reset to allow retry
            m_operationState = OperationState::COMPLETED; // Stay connected for retry
        }
        
    } catch (const std::exception& e) {
        AddLogMessage("Error parsing transfer parameters: " + std::string(e.what()), 2);
        m_progress.current_operation = "Transfer failed";
        
        // Keep connection but reset to allow retry
        m_operationState = OperationState::COMPLETED; // Stay connected for retry
    }
}

void RTEgetDataGUI::DisconnectFromTarget() {
    if (!m_isConnected) {
        return;
    }
    
    AddLogMessage("Disconnecting from target...");
    
    if (m_connectionType == ConnectionType::GDB_SERVER) {
        gdb_detach();
        AddLogMessage("Disconnected from GDB server");
    } else {
        com_close();
        AddLogMessage("Closed COM port");
    }
    
    m_isConnected = false;
    m_operationState = OperationState::IDLE;
    m_progress.current_operation = "";
    m_progress.progress = 0.0f;
    m_progress.bytes_transferred = 0;
    m_progress.total_bytes = 0;
}

void RTEgetDataGUI::AddLogMessage(const std::string& message, int level) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    
    LogEntry entry;
    entry.message = message;
    entry.timestamp = GetCurrentTimestamp();
    entry.level = level;
    
    m_logMessages.push_back(entry);
    
    // Limit log size
    if (m_logMessages.size() > 1000) {
        m_logMessages.erase(m_logMessages.begin());
    }
}

void RTEgetDataGUI::ClearLog() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_logMessages.clear();
}

std::string RTEgetDataGUI::FormatFileSize(size_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB" };
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return oss.str();
}

std::string RTEgetDataGUI::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void RTEgetDataGUI::BackgroundWorker() {
    while (m_backgroundRunning && !m_shouldStop) {
        std::function<void()> task;
        
        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            if (m_currentTask) {
                task = m_currentTask;
                m_currentTask = nullptr;
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                AddLogMessage("Error: " + std::string(e.what()), 2);
                m_operationState = OperationState::ERROR;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void RTEgetDataGUI::StartBackgroundTask(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(m_taskMutex);
    m_currentTask = task;
}

void RTEgetDataGUI::LoadSettings() {
    // TODO: Implement settings loading from file
    AddLogMessage("Default settings loaded");
}

void RTEgetDataGUI::SaveSettings() {
    // TODO: Implement settings saving to file
    AddLogMessage("Settings saved");
}

std::vector<RTEgetDataGUI::ComPortInfo> RTEgetDataGUI::EnumerateComPorts() {
    std::vector<ComPortInfo> ports;
    
#ifdef _WIN32
    // Windows implementation using SetupAPI
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return ports;
    }

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
        ComPortInfo port;
        
        // Get device name
        HKEY hKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (hKey != INVALID_HANDLE_VALUE) {
            char portName[256];
            DWORD size = sizeof(portName);
            if (RegQueryValueExA(hKey, "PortName", NULL, NULL, (LPBYTE)portName, &size) == ERROR_SUCCESS) {
                port.device = portName;
            }
            RegCloseKey(hKey);
        }
        
        // Get friendly name/description
        char buffer[256];
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_FRIENDLYNAME, 
                                             NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
            port.friendly_name = buffer;
            port.description = buffer;
        }
        
        if (!port.device.empty()) {
            ports.push_back(port);
        }
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    
#else
    // Linux implementation - scan /dev for serial devices
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string deviceName = entry->d_name;
            std::string devicePath = "/dev/" + deviceName;
            
            // Check for common serial device patterns
            if (deviceName.find("ttyUSB") == 0 || deviceName.find("ttyACM") == 0 || 
                deviceName.find("ttyS") == 0 || deviceName.find("ttyAMA") == 0 ||
                deviceName.find("rfcomm") == 0) {
                
                struct stat st;
                if (stat(devicePath.c_str(), &st) == 0 && S_ISCHR(st.st_mode)) {
                    ComPortInfo port;
                    port.device = devicePath;
                    
                    // Try to get device description from udev or sysfs
                    std::string description = GetLinuxDeviceDescription(devicePath);
                    if (description.empty()) {
                        if (deviceName.find("ttyUSB") == 0) {
                            description = "USB Serial Device";
                        } else if (deviceName.find("ttyACM") == 0) {
                            description = "USB Modem/ACM Device";
                        } else if (deviceName.find("ttyS") == 0) {
                            description = "Serial Port";
                        } else if (deviceName.find("ttyAMA") == 0) {
                            description = "ARM Serial Port";
                        } else if (deviceName.find("rfcomm") == 0) {
                            description = "Bluetooth Serial";
                        } else {
                            description = "Serial Device";
                        }
                    }
                    
                    port.description = description;
                    port.friendly_name = deviceName + " (" + description + ")";
                    ports.push_back(port);
                }
            }
        }
        closedir(dir);
    }
    
    // Sort ports by device name for consistent ordering
    std::sort(ports.begin(), ports.end(), 
              [](const ComPortInfo& a, const ComPortInfo& b) {
                  return a.device < b.device;
              });
#endif

    return ports;
}

void RTEgetDataGUI::RefreshComPorts() {
    m_availableComPorts = EnumerateComPorts();
    m_comPortsNeedRefresh = false;
    
    // Reset selection if current selection is invalid
    if (m_selectedComPortIndex >= static_cast<int>(m_availableComPorts.size())) {
        m_selectedComPortIndex = 0;
    }
    
    AddLogMessage("Found " + std::to_string(m_availableComPorts.size()) + " serial port(s)");
}

#ifndef _WIN32
std::string RTEgetDataGUI::GetLinuxDeviceDescription(const std::string& devicePath) {
    // Extract device name from path (e.g., /dev/ttyUSB0 -> ttyUSB0)
    std::string deviceName = devicePath.substr(5); // Remove "/dev/"
    
    // Try to read from sysfs
    std::vector<std::string> sysfsPaths = {
        "/sys/class/tty/" + deviceName + "/device/interface",
        "/sys/class/tty/" + deviceName + "/device/product",
        "/sys/class/tty/" + deviceName + "/device/../interface",
        "/sys/class/tty/" + deviceName + "/device/../product"
    };
    
    for (const auto& path : sysfsPaths) {
        std::ifstream file(path);
        if (file.good()) {
            std::string description;
            if (std::getline(file, description) && !description.empty()) {
                // Clean up the description
                description.erase(description.find_last_not_of(" \n\r\t") + 1);
                return description;
            }
        }
    }
    
    return "";
}
#endif

