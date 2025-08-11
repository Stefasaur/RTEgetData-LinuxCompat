/*
 * Copyright (c) Branko Premzel.
 *
 * SPDX-License-Identifier: MIT
 */

/***
 * @file      RTEgetData_GUI.h
 * @brief     GUI interface for RTEgetData using Dear ImGui
 * @author    B. Premzel
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

// Forward declarations
#ifdef GUI_USE_SDL2
    struct SDL_Window;
    union SDL_Event;
    typedef void *SDL_GLContext;
    #define WINDOW_TYPE SDL_Window*
    #define EVENT_TYPE SDL_Event*
#else
    struct GLFWwindow;
    #define WINDOW_TYPE GLFWwindow*
    #define EVENT_TYPE void*
#endif
struct ImGuiIO;

class RTEgetDataGUI {
public:
    RTEgetDataGUI();
    ~RTEgetDataGUI();

    // Main application lifecycle
    bool Initialize();
    void Run();
    void Shutdown();

private:
    // GUI State Management
    enum class ConnectionType { GDB_SERVER, COM_PORT };
    enum class OperationState { IDLE, CONNECTING, TRANSFERRING, COMPLETED, ERROR };

    // GUI Windows
    void ShowMainWindow();
    void ShowAboutWindow();

    // Connection Management
    void ConnectToTarget();
    void DisconnectFromTarget();
    void TransferData();
    void UpdateConnectionStatus();
    
    // Connection state
    bool m_isConnected = false;

    // Utility Functions
    void LoadSettings();
    void SaveSettings();
    void AddLogMessage(const std::string& message, int level = 0);
    void ClearLog();
    std::string FormatFileSize(size_t bytes);
    std::string GetCurrentTimestamp();
    
    // COM Port Detection
    struct ComPortInfo {
        std::string device;
        std::string description;
        std::string friendly_name;
    };
    std::vector<ComPortInfo> EnumerateComPorts();
    void RefreshComPorts();
#ifndef _WIN32
    std::string GetLinuxDeviceDescription(const std::string& devicePath);
#endif

    // Threading
    void BackgroundWorker();
    void StartBackgroundTask(std::function<void()> task);

private:
    // Window/OpenGL
    WINDOW_TYPE m_window = nullptr;
#ifdef GUI_USE_SDL2
    SDL_GLContext m_glContext = nullptr;
#endif
    bool m_initialized = false;

    // GUI State
    ConnectionType m_connectionType = ConnectionType::GDB_SERVER;
    OperationState m_operationState = OperationState::IDLE;
    bool m_showDemo = false;
    bool m_showAbout = false;
    
    // Connection Settings
    struct {
        char gdb_ip[256] = "127.0.0.1";
        int gdb_port = 3333;
        char com_port[256] = "COM1";
        int com_baudrate = 115200;
        int com_parity = 0;      // 0=None, 1=Odd, 2=Even
        int com_stopbits = 1;    // 1 or 2
        int com_timeout = 50;
        bool single_wire = false;
    } m_settings;

    // Transfer Settings
    struct {
        char address[32] = "0x24000000";
        char size[32] = "0x2000";
        char output_file[512] = "data.bin";
        char filter_file[512] = "";
        bool clear_buffer = false;
        bool persistent_mode = false;
        int delay_ms = 0;
        char decode_script[512] = "";
    } m_transfer;

    // Progress Tracking
    struct {
        float progress = 0.0f;
        std::string current_operation;
        std::string status_message;
        size_t bytes_transferred = 0;
        size_t total_bytes = 0;
        std::chrono::steady_clock::time_point start_time;
    } m_progress;

    // Logging
    struct LogEntry {
        std::string message;
        std::string timestamp;
        int level; // 0=info, 1=warning, 2=error
    };
    std::vector<LogEntry> m_logMessages;
    std::mutex m_logMutex;
    bool m_autoScroll = true;
    bool m_showTimestamps = true;
    int m_logLevelFilter = 0; // Show all levels >= this

    // Background Threading
    std::thread m_backgroundThread;
    std::atomic<bool> m_backgroundRunning{false};
    std::atomic<bool> m_shouldStop{false};
    std::mutex m_taskMutex;
    std::function<void()> m_currentTask;

    // Recent Connections
    struct RecentConnection {
        std::string name;
        ConnectionType type;
        std::string details; // IP:port or COM port
        std::string address;
        std::string size;
    };
    std::vector<RecentConnection> m_recentConnections;
    static constexpr size_t MAX_RECENT_CONNECTIONS = 10;

    // File Dialog State
    std::string m_currentDirectory;
    bool m_showFileDialog = false;
    bool m_fileDialogForOutput = true; // true=output file, false=filter file
    
    // COM Port Detection State
    std::vector<ComPortInfo> m_availableComPorts;
    int m_selectedComPortIndex = 0;
    bool m_comPortsNeedRefresh = true;
};

/*==== End of file ====*/