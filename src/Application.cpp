#include "Application.hpp"
#include "Communication.hpp"
#include "InfoWindow.hpp"
#include "MainWindow.hpp"
#include <fstream>
#include <future>
#include <iomanip>
#include <mygui/ResourceManager.hpp>
#include <sstream>

using namespace std::chrono_literals;

void Application::Information()
{
    int     detected_in_window_cnt = 0, detected_out_window_cnt = 0, signal_missed_cnt = 0;
    auto    time_at_start = std::chrono::steady_clock::now();
    int64_t run_sec       = 0;

    while (m_mainWindow->IsOpen()) {
        // Output number of received packets
        m_mainWindow->label_info_rx_id_avail->SetText("Rx id: " + std::to_string(m_rcv_packet_id) + " available: " + std::to_string(m_available_bytes) + " bytes");
        m_mainWindow->label_info_rx_time_took_speed->SetText("Rx took: " + std::to_string(m_time_took_to_read_data_us / 1000) + " ms at: " + std::to_string(m_comm_speed_kb_s) + " kB/s");
        std::stringstream ss;
        ss << std::setw(4) << m_time_took_to_parse_data_us;
        m_mainWindow->label_info_parse_data_time->SetText("Parsing data took: " + ss.str() + " us");

        detected_in_window_cnt = detected_out_window_cnt = signal_missed_cnt = 0;
        for (const auto& s : m_mainWindow->signals) {
            detected_in_window_cnt += s->GetDetectionsInWindow();
            detected_out_window_cnt += s->GetDetectionsOutWindow();
            signal_missed_cnt += s->GetMissed();
        }
        m_mainWindow->label_info_detected_in_window->SetText(std::to_string(detected_in_window_cnt));
        m_mainWindow->label_info_detected_out_window->SetText(std::to_string(detected_out_window_cnt));
        m_mainWindow->label_info_signal_missed->SetText(std::to_string(signal_missed_cnt));

        auto time_now  = std::chrono::steady_clock::now();
        auto alive_sec = std::chrono::duration_cast<std::chrono::seconds>(time_now - time_at_start).count();

        if (*m_running)
            run_sec = std::chrono::duration_cast<std::chrono::seconds>(time_now - m_mainWindow->GetRunStartTime()).count();

        int size     = m_mainWindow->signals.size() * m_mainWindow->signals[0]->GetRXData().size() * sizeof(m_mainWindow->signals[0]->GetRXData()[0]) / 1000000;
        int capacity = m_mainWindow->signals.size() * m_mainWindow->signals[0]->GetRXData().capacity() * sizeof(m_mainWindow->signals[0]->GetRXData()[0]) / 1000000;

        std::stringstream str;
        str << "Sorting Control    alive: " << std::to_string(alive_sec / 60) << ":" << std::setw(2) << std::setfill('0') << std::to_string(alive_sec % 60)
            << "  running: " << std::to_string(run_sec / 60) << ":" << std::setw(2) << std::setfill('0') << std::to_string(run_sec % 60) << "   Buffer size: " << size << " MB"
            << " / " << capacity << " MB";
        m_mainWindow->SetTitle(str.str());

        std::this_thread::sleep_for(200ms);
    }
}

void Application::GetData()
{
    Header header         = {0, 0};
    auto   prev_packet_id = header.packet_id;

    std::future<void> future           = std::async(std::launch::async, [] { return; }); // create a valid future
    std::atomic_bool  parsing_too_slow = false;

    while (m_mainWindow->IsOpen()) {

        if (*m_running) {

            header.delim = 0; // reset value

            m_available_bytes = m_communication->GetRxBufferLen();

            m_communication->Read(&header, sizeof(header.delim));
            // If start of new packet
            if (header.delim == 0xDEADBEEF) {

                // Check header for valid packet ID
                m_communication->Read(&header.packet_id, sizeof(header.packet_id));
                if (header.packet_id != (prev_packet_id + 1)) // if we missed a packet
                    std::cerr << "Packet(s) lost: Should receive: " << prev_packet_id + 1 << " received: " << header.packet_id << ". Info: previous packet took: " << m_time_took_to_read_data_us / 1000 << " ms to read." << std::endl;

                // Remember latest packet ID
                prev_packet_id  = header.packet_id;
                m_rcv_packet_id = header.packet_id;

                auto start = std::chrono::high_resolution_clock::now();

                size_t read = m_communication->Read(m_data, sizeof(m_data));

                if (read == sizeof(m_data)) {
                    if (future.wait_for(0ms) == std::future_status::ready) {
                        future = std::async(std::launch::async, [this, &parsing_too_slow] {
                            // Time it
                            auto start = std::chrono::steady_clock::now();
                            // Internal tmp data buffer
                            ProtocolDataType data_tmp_buf[sizeof(m_data)];
                            // Copy to tmp buffer
                            std::memcpy(data_tmp_buf, m_data, sizeof(m_data));
                            // Update signals
                            m_mainWindow->UpdateSignals(data_tmp_buf);
                            // How long did it all take
                            m_time_took_to_parse_data_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
                            // If parsing was too slow output how long it took
                            if (parsing_too_slow) {
                                parsing_too_slow = false;
                                std::cerr << "Parsing took: " << m_time_took_to_parse_data_us << " us\n";
                            }
                            return;
                        });
                    } else {
                        parsing_too_slow = true;
                        std::cerr << "Data parsing too slow: overwritten previous packet.\n";
                    }

                } else {
                    std::cerr << "Read insufficent bytes. Read " << read << " bytes insted " << sizeof(m_data) << "\n";
                }

                auto finished               = std::chrono::high_resolution_clock::now();
                m_time_took_to_read_data_us = std::chrono::duration_cast<std::chrono::microseconds>(finished - start).count();
                m_comm_speed_kb_s           = (read * 1000) / m_time_took_to_read_data_us; // kB/s
            }
        }

        if (!*m_running)
            prev_packet_id = 0;
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
            m_config_com_port = tokens[i];
            break;
        case 1: // number of samples
            m_config_number_of_samples = std::stoi(tokens[i]);
            break;
        }
    }
}

void Application::Run()
{
    // Create threads that are the brains of the program
    m_thread_info     = std::thread(std::bind(&Application::Information, this));
    m_thread_get_data = std::thread(std::bind(&Application::GetData, this));

    while (m_mainWindow->IsOpen()) {
        m_mainWindow->Update();
        m_detectionInfoWindow->Update();
        m_frameInfoWindow->Update();
        // 60 FPS is enough
        std::this_thread::sleep_for(15ms);
    }
    m_detectionInfoWindow->Close();
    m_frameInfoWindow->Close();
}

Application::Application()
{
    // Initial parameters from file init
    InitFromFile("config.txt");

    // Set resource manager font name
    mygui::ResourceManager::SetSystemFontName("segoeui.ttf");

    // Set state variables
    m_running = std::make_shared<bool>(false);
    m_record  = std::make_shared<Record>(Record::NO);

    // Create communication
    m_communication = std::make_shared<Communication>();

    // Create main window
    m_mainWindow = std::make_unique<MainWindow>(1850, 900, "Sorting Control", m_config_com_port, m_config_number_of_samples, sf::Style::None | sf::Style::Close);

    // Create detection info window
    m_detectionInfoWindow = std::make_shared<InfoWindow>("Detection Info", "det.py");
    m_detectionInfoWindow->SetPosition(m_mainWindow->GetPosition() + sf::Vector2i(1850 - 480, 40));
    m_detectionInfoWindow->SetSampleFrequency(m_mainWindow->GetSampleFreq());
    m_detectionInfoWindow->SetVisible(false);
    for (auto& s : m_mainWindow->signals) {
        m_detectionInfoWindow->push_back(s->GetDetectionStats());
    }
    //m_detectionInfoWindow->SetAll(Signal::GetDetectionStatsAll());

    // Create frame info window
    m_frameInfoWindow = std::make_shared<InfoWindow>("Frame Info", "win.py");
    m_frameInfoWindow->SetPosition(m_mainWindow->GetPosition() + sf::Vector2i(1850 - 1000, 40));
    m_frameInfoWindow->SetSampleFrequency(m_mainWindow->GetSampleFreq());
    m_frameInfoWindow->SetVisible(false);
    for (auto& s : m_mainWindow->signals) {
        m_frameInfoWindow->push_back(s->GetTriggerWindowStats());
    }
    //m_frameInfoWindow->SetAll(Signal::GetTriggerWindowStatsAll());

    // Now that all objects are created pass all neccessary data to mainwindow
    m_mainWindow->ConnectCrossData(m_communication, m_detectionInfoWindow, m_frameInfoWindow, m_running, m_record);
}

Application::~Application()
{
    m_thread_info.join();
    m_thread_get_data.join();
}