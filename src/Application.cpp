#include "Application.hpp"
#include "Device.hpp"
#include "InfoWindow.hpp"
#include "MainWindow.hpp"
#include <fstream>
#include <future>
#include <iomanip>
#include <mygui/ResourceManager.hpp>
#include <sstream>

using namespace std::chrono_literals;

static void ListPorts()
{
    Communication comm;
    comm.SetTimeout(100);

    // Find only free ports
    auto all_ports  = comm.ListAllPorts();
    auto free_ports = comm.ListFreePorts();
    for (auto it = all_ports.begin(); it != all_ports.end();) {
        bool found = false;
        for (auto fp = free_ports.begin(); fp != free_ports.end(); ++fp)
            if (it->port == *fp)
                found = true;

        if (!found)
            it = all_ports.erase(it);
        else
            ++it;
    }

    // Extract ports of valid STM32 devices by checking description
    decltype(all_ports) ports;
    for (auto it = all_ports.begin(); it != all_ports.end(); ++it)
        if (it->description.find("STMicroelectronics Virtual COM Port") != std::string::npos) // found it
            ports.push_back(*it);

    if (ports.empty()) {
        std::cout << "No available serial ports found!\n";
        return;
    }

    std::map<int, std::pair<std::string, std::string>> port_map;

    for (auto const& [p, desc, hw_id] : ports) {
        if (comm.Connect(p)) {
            comm.StopTransmissionAndSuperPurge();
            auto tok = comm.WriteAndTokenizeResult("ID_G\n");
            if (tok.size() == 1)
                port_map[std::stoi(tok[0])] = std::make_pair(p, desc);

            comm.Disconnect();
        }
    }

    for (auto const& [id, pair] : port_map)
        std::cout << pair.first << "(" << pair.second << "): ID_G," << id << std::endl;
}

void Application::Information()
{
    int     detected_in_window_cnt = 0, detected_out_window_cnt = 0, signal_missed_cnt = 0;
    auto    time_at_start = std::chrono::steady_clock::now();
    int64_t run_sec       = 0;

    while (m_main_window->IsOpen()) {
        // Output number of received packets
        m_main_window->label_info_rx_id_avail->SetText("Rx id: " + std::to_string(m_rcv_packet_id) + " available: " + std::to_string(m_available_bytes) + " bytes");
        m_main_window->label_info_rx_time_took_speed->SetText("Rx took: " + std::to_string(m_time_took_to_read_data_us / 1000) + " ms at: " + std::to_string(m_comm_speed_kb_s) + " kB/s");
        std::stringstream ss;
        ss << std::setw(4) << m_time_took_to_parse_data_us;
        m_main_window->label_info_parse_data_time->SetText("Parsing data took: " + ss.str() + " us");

        detected_in_window_cnt = detected_out_window_cnt = signal_missed_cnt = 0;
        for (const auto& s : m_main_window->signals) {
            detected_in_window_cnt += s->GetDetectionsInWindow();
            detected_out_window_cnt += s->GetDetectionsOutWindow();
            signal_missed_cnt += s->GetMissed();
        }
        m_main_window->label_info_detected_in_window->SetText(std::to_string(detected_in_window_cnt));
        m_main_window->label_info_detected_out_window->SetText(std::to_string(detected_out_window_cnt));
        m_main_window->label_info_signal_missed->SetText(std::to_string(signal_missed_cnt));

        auto time_now  = std::chrono::steady_clock::now();
        auto alive_sec = std::chrono::duration_cast<std::chrono::seconds>(time_now - time_at_start).count();

        if (*m_running)
            run_sec = std::chrono::duration_cast<std::chrono::seconds>(time_now - m_main_window->GetRunStartTime()).count();

        int size     = m_main_window->signals.size() * m_main_window->signals[0]->GetRXData().size() * sizeof(m_main_window->signals[0]->GetRXData()[0]) / 1000000;
        int capacity = m_main_window->signals.size() * m_main_window->signals[0]->GetRXData().capacity() * sizeof(m_main_window->signals[0]->GetRXData()[0]) / 1000000;

        std::stringstream str;
        str << "Sorting Control    alive: " << std::to_string(alive_sec / 60) << ":" << std::setw(2) << std::setfill('0') << std::to_string(alive_sec % 60)
            << "  running: " << std::to_string(run_sec / 60) << ":" << std::setw(2) << std::setfill('0') << std::to_string(run_sec % 60) << "   Buffer size: " << size << " MB"
            << " / " << capacity << " MB";
        m_main_window->SetTitle(str.str());

        std::this_thread::sleep_for(200ms);
    }
}

void Application::GetData()
{
    Header header         = {0, 0};
    auto   prev_packet_id = header.packet_id;

    std::future<void> future           = std::async(std::launch::async, [] { return; }); // create a valid future
    std::atomic_bool  parsing_too_slow = false;

    while (m_main_window->IsOpen()) {

        if (*m_running) {

            header.delim = 0; // reset value

            m_available_bytes = m_communication1->GetRxBufferLen();

            m_communication1->Read(&header, sizeof(header.delim));
            // If start of new packet
            if (header.delim == 0xDEADBEEF) {

                // Check header for valid packet ID
                m_communication1->Read(&header.packet_id, sizeof(header.packet_id));
                if (header.packet_id != (prev_packet_id + 1)) // if we missed a packet
                    std::cerr << "Packet(s) lost: Should receive: " << prev_packet_id + 1 << " received: " << header.packet_id << ". Info: previous packet took: " << m_time_took_to_read_data_us / 1000 << " ms to read." << std::endl;

                // Remember latest packet ID
                prev_packet_id  = header.packet_id;
                m_rcv_packet_id = header.packet_id;

                auto start = std::chrono::high_resolution_clock::now();

                size_t read = m_communication1->Read(m_data, sizeof(m_data));

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
                            m_main_window->UpdateSignals(data_tmp_buf);
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

void Application::Run()
{
    // Create threads that are the brains of the program
    m_thread_info     = std::thread(std::bind(&Application::Information, this));
    m_thread_get_data = std::thread(std::bind(&Application::GetData, this));

    while (m_main_window->IsOpen()) {
        m_main_window->Update();
        m_info_win_det_sc_top->Update();
        m_info_win_frm_sc_top->Update();
        // 60 FPS is enough
        std::this_thread::sleep_for(15ms);
    }
    m_info_win_det_sc_top->Close();
    m_info_win_frm_sc_top->Close();
}

Application::Application()
{
    ListPorts();

    // Set resource manager font name
    mygui::ResourceManager::SetSystemFontName("segoeui.ttf");

    // Set state variables
    m_running = std::make_shared<bool>(false);
    m_record  = std::make_shared<Record>(Record::NO);

    // Create communication
    m_dev_sc_top    = std::make_shared<Device>(2);
    m_dev_sc_bottom = std::make_shared<Device>(3);

    // Create main window
    m_main_window = std::make_unique<MainWindow>(1850, 900, "Sorting Control", m_config_com_port, m_config_number_of_samples, sf::Style::None | sf::Style::Close);

    // Create detection info window
    m_info_win_det_sc_top = std::make_shared<InfoWindow>("Detection Info", "det.py");
    m_info_win_det_sc_top->SetPosition(m_main_window->GetPosition() + sf::Vector2i(1850 - 480, 40));
    m_info_win_det_sc_top->SetSampleFrequency(m_main_window->GetSampleFreq());
    m_info_win_det_sc_top->SetVisible(false);
    for (auto& s : m_main_window->signals) {
        m_info_win_det_sc_top->push_back(s->GetDetectionStats());
    }
    //m_detectionInfoWindow->SetAll(Signal::GetDetectionStatsAll());

    // Create frame info window
    m_info_win_frm_sc_top = std::make_shared<InfoWindow>("Frame Info", "win.py");
    m_info_win_frm_sc_top->SetPosition(m_main_window->GetPosition() + sf::Vector2i(1850 - 1000, 40));
    m_info_win_frm_sc_top->SetSampleFrequency(m_main_window->GetSampleFreq());
    m_info_win_frm_sc_top->SetVisible(false);
    for (auto& s : m_main_window->signals) {
        m_info_win_frm_sc_top->push_back(s->GetTriggerWindowStats());
    }
    //m_frameInfoWindow->SetAll(Signal::GetTriggerWindowStatsAll());

    // Now that all objects are created pass all neccessary data to mainwindow
    m_main_window->ConnectCrossData(m_communication1, m_info_win_det_sc_top, m_info_win_frm_sc_top, m_running, m_record);
}

Application::~Application()
{
    m_thread_info.join();
    m_thread_get_data.join();
}