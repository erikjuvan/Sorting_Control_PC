#include "Application.hpp"
#include "Communication.hpp"
#include "InfoWindow.hpp"
#include "MainWindow.hpp"
#include <fstream>
#include <iomanip>
#include <thread>

Communication* g_communication;
MainWindow*    g_mainWindow;
InfoWindow*    g_detectionInfoWindow;
InfoWindow*    g_frameInfoWindow;

Running g_running;
Record  g_record;
View    g_view;

static std::thread g_thread_info;
static std::thread g_thread_get_data;

static void Information()
{
    static int cnt                    = 0;
    static int detected_in_window_cnt = 0, detected_out_window_cnt = 0, signal_missed_cnt = 0;
    auto       time_at_start = std::chrono::steady_clock::now();

    while (g_mainWindow->IsOpen()) {
        if (g_communication->IsConnected()) {
            g_mainWindow->label_info_rx_bytes->SetText(std::to_string(cnt++) + " Rx buf: " + std::to_string(g_communication->GetRxBufferLen()) + " bytes");

            detected_in_window_cnt = detected_out_window_cnt = signal_missed_cnt = 0;
            for (const auto& s : g_mainWindow->signals) {
                detected_in_window_cnt += s.GetDetectionsInWindow();
                detected_out_window_cnt += s.GetDetectionsOutWindow();
                signal_missed_cnt += s.GetMissed();
            }
            g_mainWindow->label_info_detected_in_window->SetText(std::to_string(detected_in_window_cnt));
            g_mainWindow->label_info_detected_out_window->SetText(std::to_string(detected_out_window_cnt));
            g_mainWindow->label_info_signal_missed->SetText(std::to_string(signal_missed_cnt));
        }

        auto time_now  = std::chrono::steady_clock::now();
        auto alive_sec = std::chrono::duration_cast<std::chrono::seconds>(time_now - time_at_start).count();

        static uint64_t run_sec = 0;
        if (g_running == Running::RUNNING)
            run_sec = std::chrono::duration_cast<std::chrono::seconds>(time_now - Application::run_start_time).count();

        int               size     = g_mainWindow->signals.size() * g_mainWindow->signals[0].GetRawData().size() * sizeof(g_mainWindow->signals[0].GetRawData()[0]) / 1000000;
        int               capacity = g_mainWindow->signals.size() * g_mainWindow->signals[0].GetRawData().capacity() * sizeof(g_mainWindow->signals[0].GetRawData()[0]) / 1000000;
        std::stringstream str;
        str << "Sorting Control    alive: " << std::to_string(alive_sec / 60) << ":" << std::setw(2) << std::setfill('0') << std::to_string(alive_sec % 60)
            << "  running: " << std::to_string(run_sec / 60) << ":" << std::setw(2) << std::setfill('0') << std::to_string(run_sec % 60) << "   Buffer size: " << size << " MB"
            << " / " << capacity << " MB";
        g_mainWindow->SetTitle(str.str());

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
}

static void GetData()
{
    static float fbuf[N_CHANNELS * DATA_PER_CHANNEL];
    static int   cntr    = 0;
    bool         running = true;

    while (g_mainWindow->IsOpen()) {

        if (g_communication->IsConnected() &&
            g_running == Running::RUNNING) {

            g_communication->Read(fbuf, 4);
            uint32_t delim = *((uint32_t*)&fbuf[0]);

            if (delim == 0xDEADBEEF) { // Data
                size_t read = g_communication->Read(fbuf, sizeof(fbuf));
                if (read > 0) {
                    float* fbuf_tmp = fbuf;
                    for (auto& s : g_mainWindow->signals) {
                        s.Edit(fbuf_tmp, cntr * DATA_PER_CHANNEL, DATA_PER_CHANNEL);
                        fbuf_tmp += DATA_PER_CHANNEL;
                    }
                    if (++cntr >= (Application::config_number_of_samples / DATA_PER_CHANNEL)) {
                        cntr = 0;
                        if (g_record == Record::ALL) {
                            for (auto const& s : g_mainWindow->signals)
                                g_mainWindow->recorded_signals.push_back(s);
                            g_mainWindow->label_recorded_signals_counter->SetText(std::to_string(g_mainWindow->recorded_signals.size() / N_CHANNELS));
                        } else if (g_record == Record::EVENTS) {
                            bool event_happened = false;
                            for (auto const& s : g_mainWindow->signals) {
                                if (s.AnyEvents()) {
                                    event_happened = true;
                                    break;
                                }
                            }

                            if (event_happened) {
                                for (auto& s : g_mainWindow->signals) {
                                    if (s.AnyEvents()) {
                                        g_mainWindow->recorded_signals.push_back(s);
                                        s.ClearEvents();
                                    } else {
                                        g_mainWindow->recorded_signals.push_back(Signal()); // push empty signal
                                    }
                                }
                                g_mainWindow->label_recorded_signals_counter->SetText(std::to_string(g_mainWindow->recorded_signals.size() / N_CHANNELS));
                            }
                        }

                        if (g_frameInfoWindow)
                            g_frameInfoWindow->RefreshTable();
                        if (g_detectionInfoWindow)
                            g_detectionInfoWindow->RefreshTable();
                    }
                }
            }
        }
    }
}

void Application::InitFromFile(const std::string& file_name)
{
    std::ifstream            in_file(file_name);
    std::string              str;
    std::vector<std::string> tokens;
    if (in_file.is_open()) {
        while (std::getline(in_file, str)) {
            tokens.push_back(str);
        }
        in_file.close();
    }

    for (int i = 0; i < tokens.size(); ++i) {
        switch (i) {
        case 0: // COM port
            Application::config_com_port = tokens[i];
            break;
        case 1: // number of samples
            Application::config_number_of_samples = std::stoi(tokens[i]);
            break;
        }
    }
}

void Application::Init()
{
    // Initial parameters from file init
    InitFromFile("config.txt");

    g_communication = new Communication();
    g_mainWindow    = new MainWindow(1850, 900, "Sorting Control", sf::Style::None | sf::Style::Close);

    g_detectionInfoWindow = new InfoWindow("Detection Info", "det.py");
    g_detectionInfoWindow->SetPosition(g_mainWindow->GetPosition() + sf::Vector2i(1850 - 480, 40));
    for (auto& s : g_mainWindow->signals) {
        g_detectionInfoWindow->push_back(&s.GetDetecionStats());
    }
    g_detectionInfoWindow->SetAll(Signal::GetDetecionStatsAll());

    g_frameInfoWindow = new InfoWindow("Frame Info", "win.py");
    g_frameInfoWindow->SetPosition(g_mainWindow->GetPosition() + sf::Vector2i(1850 - 1000, 40));
    for (auto& s : g_mainWindow->signals) {
        g_frameInfoWindow->push_back(&s.GetTriggerWindowStats());
    }
    g_frameInfoWindow->SetAll(Signal::GetTriggerWindowStatsAll());

    g_running = Running::STOPPED;
    g_record  = Record::NO;
    g_view    = View::FILTERED;

    g_thread_info     = std::thread(Information);
    g_thread_get_data = std::thread(GetData);
}

void Application::Run()
{
    while (g_mainWindow->IsOpen()) {
        g_mainWindow->Update();
        g_detectionInfoWindow->Update();
        g_frameInfoWindow->Update();
    }

    g_thread_info.join();
    g_thread_get_data.join();

    delete g_detectionInfoWindow;
    delete g_frameInfoWindow;
    delete g_mainWindow;
    delete g_communication;
}
