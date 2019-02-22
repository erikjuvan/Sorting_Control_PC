#include "Application.hpp"
#include "Communication.hpp"
#include "InfoWindow.hpp"
#include "MainWindow.hpp"
#include <atomic>
#include <fstream>
#include <iomanip>
#include <thread>

Communication* g_communication;
MainWindow*    g_mainWindow;
InfoWindow*    g_detectionInfoWindow;
InfoWindow*    g_frameInfoWindow;

Running      g_running      = Running::STOPPED;
Record       g_record       = Record::NO;
View         g_view         = View::FILTERED;
TriggerFrame g_triggerframe = TriggerFrame::ON;

static std::thread g_thread_info;
static std::thread g_thread_get_data;
static std::thread g_thread_parse_data;

static int rcv_packet_cntr = 0; // it should be atomic but it is not neccessary since it's just informative counter

static ProtocolDataType g_data[N_CHANNELS * DATA_PER_CHANNEL];
static std::atomic_bool g_data_in_buffer = false;

static void Information()
{
    int  detected_in_window_cnt = 0, detected_out_window_cnt = 0, signal_missed_cnt = 0;
    auto time_at_start = std::chrono::steady_clock::now();

    while (g_mainWindow->IsOpen()) {
        if (g_communication->IsConnected()) {
            // Output number of received packets
            g_mainWindow->label_info_rx_bytes->SetText(std::to_string(rcv_packet_cntr) + " Rx buf: " + std::to_string(g_communication->GetRxBufferLen()) + " bytes");

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

        int size     = g_mainWindow->signals.size() * g_mainWindow->signals[0].GetRXData().size() * sizeof(g_mainWindow->signals[0].GetRXData()[0]) / 1000000;
        int capacity = g_mainWindow->signals.size() * g_mainWindow->signals[0].GetRXData().capacity() * sizeof(g_mainWindow->signals[0].GetRXData()[0]) / 1000000;

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
    Header header          = {0, 0};
    auto   prev_packet_id  = header.packet_id;
    bool   packet_received = false;

    while (g_mainWindow->IsOpen()) {

        if (g_communication->IsConnected() && g_running == Running::RUNNING) {

            g_communication->Read(&header, sizeof(header.delim));
            // If start of new packet
            if (header.delim == 0xDEADBEEF) {

                // Check header for valid packet ID
                g_communication->Read(&header.packet_id, sizeof(header.packet_id));
                if (header.packet_id != (prev_packet_id + 1)) // if we missed a packet
                    std::cerr << "Packets lost: Should receive: " << prev_packet_id + 1 << " received: " << header.packet_id << std::endl;

                // Remember latest packet ID
                prev_packet_id = header.packet_id;

                if (g_data_in_buffer.load())
                    std::cerr << "Data parsing too slow: overwritting unparsed packet\n";

                size_t read = g_communication->Read(g_data, sizeof(g_data));
                if (read > 0)
                    g_data_in_buffer.store(true);

                // Update received packet counter
                rcv_packet_cntr++;
            }
        }
    }
}

static void ParseData()
{
    ProtocolDataType data_buf[sizeof(g_data)];
    int              cntr = 0;

    while (g_mainWindow->IsOpen()) {

        if (g_data_in_buffer.load()) {
            std::memcpy(data_buf, g_data, sizeof(g_data));
            g_data_in_buffer.store(false);
            ProtocolDataType* p_data = data_buf;
            // Update signals with new data
            for (auto& s : g_mainWindow->signals) {
                s.Edit(p_data, cntr * DATA_PER_CHANNEL, DATA_PER_CHANNEL);
                p_data += DATA_PER_CHANNEL;
            }

            // If we filled up char/signal
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

    g_thread_info       = std::thread(Information);
    g_thread_get_data   = std::thread(GetData);
    g_thread_parse_data = std::thread(ParseData);
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
    g_thread_parse_data.join();

    delete g_detectionInfoWindow;
    delete g_frameInfoWindow;
    delete g_mainWindow;
    delete g_communication;
}
