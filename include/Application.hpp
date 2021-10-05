#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <optional>
#include <string>
#include <thread>

enum class Record { NO,
                    ALL,
                    EVENTS };
enum class View { RAW,
                  FILTERED };

// Forward decleration since including headers with said classes causes Application.hpp to be included last and we get bunch of "variable undefined" errors
// TODO: check into it
class MainWindow;
class InfoWindow;
class Device;

class Application
{
private:
    // Members
    std::unique_ptr<MainWindow> m_main_window;
    std::shared_ptr<InfoWindow> m_info_win_det_sc_top, m_info_win_det_sc_bottom,
        m_info_win_frm_sc_top, m_info_win_frm_sc_bottom;
    std::shared_ptr<Device>     m_dev_sc_top, m_dev_sc_bottom;

    bool   m_running;
    Record m_record;

    std::thread m_thread_info;

    // Methods
    void Information();

public:
    Application();
    ~Application();

    void Run();
};