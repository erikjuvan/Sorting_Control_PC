#include "Application.hpp"
#include "Communication.hpp"
#include "InfoWindow.hpp"
#include "MainWindow.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

void Application::Information()
{
    int  detected_in_window_cnt = 0, detected_out_window_cnt = 0, signal_missed_cnt = 0;
    auto time_at_start = std::chrono::steady_clock::now();

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

        static uint64_t run_sec = 0;
        if (*m_running == Running::RUNNING)
            run_sec = std::chrono::duration_cast<std::chrono::seconds>(time_now - m_mainWindow->GetRunStartTime()).count();

        int size     = m_mainWindow->signals.size() * m_mainWindow->signals[0]->GetRXData().size() * sizeof(m_mainWindow->signals[0]->GetRXData()[0]) / 1000000;
        int capacity = m_mainWindow->signals.size() * m_mainWindow->signals[0]->GetRXData().capacity() * sizeof(m_mainWindow->signals[0]->GetRXData()[0]) / 1000000;

        std::stringstream str;
        str << "Sorting Control    alive: " << std::to_string(alive_sec / 60) << ":" << std::setw(2) << std::setfill('0') << std::to_string(alive_sec % 60)
            << "  running: " << std::to_string(run_sec / 60) << ":" << std::setw(2) << std::setfill('0') << std::to_string(run_sec % 60) << "   Buffer size: " << size << " MB"
            << " / " << capacity << " MB";
        m_mainWindow->SetTitle(str.str());

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
}

void Application::GetData()
{
    Header header          = {0, 0};
    auto   prev_packet_id  = header.packet_id;
    bool   packet_received = false;

    while (m_mainWindow->IsOpen()) {

        m_available_bytes = m_communication->GetRxBufferLen();

        if (m_communication->IsConnected() && *m_running == Running::RUNNING) {

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

                if (m_data_in_buffer.load())
                    std::cerr << "Data parsing too slow: overwritting unparsed packet\n";

                auto   start                = std::chrono::high_resolution_clock::now();
                size_t read                 = m_communication->Read(m_data, sizeof(m_data));
                m_time_took_to_read_data_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
                m_comm_speed_kb_s           = (read * 1000) / m_time_took_to_read_data_us; // kB/s
                if (read > 0)
                    m_data_in_buffer.store(true);
            }
        }
    }
}

void Application::ParseData()
{
    ProtocolDataType data_buf[sizeof(m_data)];
    int              cntr = 0;

    while (m_mainWindow->IsOpen()) {

        if (m_data_in_buffer.load()) {
            auto start = std::chrono::high_resolution_clock::now();

            std::memcpy(data_buf, m_data, sizeof(m_data));
            m_data_in_buffer.store(false);

            ProtocolDataType* p_data = data_buf;
            m_mainWindow->UpdateSignals(p_data);

            m_time_took_to_parse_data_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
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
    while (m_mainWindow->IsOpen()) {
        m_mainWindow->Update();
        m_detectionInfoWindow->Update();
        m_frameInfoWindow->Update();
    }
}

Application::Application()
{
    // Initial parameters from file init
    InitFromFile("config.txt");

    m_running = std::make_shared<Running>(Running::STOPPED);
    m_record  = std::make_shared<Record>(Record::NO);

    m_communication = std::make_shared<Communication>();
    m_mainWindow    = std::make_unique<MainWindow>(1850, 900, "Sorting Control", m_config_com_port, m_config_number_of_samples, sf::Style::None | sf::Style::Close);
    m_mainWindow->ConnectCrossData(m_communication, m_detectionInfoWindow, m_frameInfoWindow, m_running, m_record);

    m_detectionInfoWindow = std::make_shared<InfoWindow>("Detection Info", "det.py");
    m_detectionInfoWindow->SetPosition(m_mainWindow->GetPosition() + sf::Vector2i(1850 - 480, 40));
    for (auto& s : m_mainWindow->signals) {
        m_detectionInfoWindow->push_back(s->GetDetectionStats());
    }
    //m_detectionInfoWindow->SetAll(Signal::GetDetectionStatsAll());

    m_frameInfoWindow = std::make_shared<InfoWindow>("Frame Info", "win.py");
    m_frameInfoWindow->SetPosition(m_mainWindow->GetPosition() + sf::Vector2i(1850 - 1000, 40));
    for (auto& s : m_mainWindow->signals) {
        m_frameInfoWindow->push_back(s->GetTriggerWindowStats());
    }
    //m_frameInfoWindow->SetAll(Signal::GetTriggerWindowStatsAll());

    m_thread_info       = std::thread(std::bind(&Application::Information, this));
    m_thread_get_data   = std::thread(std::bind(&Application::GetData, this));
    m_thread_parse_data = std::thread(std::bind(&Application::ParseData, this));
}

Application::~Application()
{
    m_thread_info.join();
    m_thread_get_data.join();
    m_thread_parse_data.join();
}