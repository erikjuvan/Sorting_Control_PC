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

        if (m_running)
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

void Application::Run()
{
    // Create threads that are the brains of the program
    m_thread_info     = std::thread(std::bind(&Application::Information, this));

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

    // Create Devices
    m_dev_sc_top    = std::make_shared<Device>(2 /* ID */);
    m_dev_sc_bottom = std::make_shared<Device>(3 /* ID */);

    // Create main window
    m_main_window = std::make_unique<MainWindow>(1850, 900, "Sorting Control", sf::Style::None | sf::Style::Close);

    // Create top detection info window
    m_info_win_det_sc_top = std::make_shared<InfoWindow>("Top Detection Info", "top_det.py");
    m_info_win_det_sc_top->SetPosition(m_main_window->GetPosition() + sf::Vector2i(1850 - 480, 40));
    m_info_win_det_sc_top->SetSampleFrequency(m_main_window->GetSampleFreq());
    m_info_win_det_sc_top->SetVisible(false);
    for (auto& s : m_main_window->signals) {
        m_info_win_det_sc_top->push_back(s->GetDetectionStats());
    }

    // Create top frame info window
    m_info_win_frm_sc_top = std::make_shared<InfoWindow>("Top Frame Info", "top_win.py");
    m_info_win_frm_sc_top->SetPosition(m_main_window->GetPosition() + sf::Vector2i(1850 - 1000, 40));
    m_info_win_frm_sc_top->SetSampleFrequency(m_main_window->GetSampleFreq());
    m_info_win_frm_sc_top->SetVisible(false);
    for (auto& s : m_main_window->signals) {
        m_info_win_frm_sc_top->push_back(s->GetTriggerWindowStats());
    }

    // Create bottom detection info window
    m_info_win_det_sc_bottom = std::make_shared<InfoWindow>("Bottom Detection Info", "bot_det.py");
    m_info_win_det_sc_bottom->SetPosition(m_main_window->GetPosition() + sf::Vector2i(1850 - 480, 440));
    m_info_win_det_sc_bottom->SetSampleFrequency(m_main_window->GetSampleFreq());
    m_info_win_det_sc_bottom->SetVisible(false);
    for (auto& s : m_main_window->signals) {
        m_info_win_det_sc_bottom->push_back(s->GetDetectionStats());
    }

    // Create bottom frame info window
    m_info_win_frm_sc_bottom = std::make_shared<InfoWindow>("Bottom Frame Info", "bot_win.py");
    m_info_win_frm_sc_bottom->SetPosition(m_main_window->GetPosition() + sf::Vector2i(1850 - 1000, 440));
    m_info_win_frm_sc_bottom->SetSampleFrequency(m_main_window->GetSampleFreq());
    m_info_win_frm_sc_bottom->SetVisible(false);
    for (auto& s : m_main_window->signals) {
        m_info_win_frm_sc_bottom->push_back(s->GetTriggerWindowStats());
    }

    // Connect signals
    m_dev_sc_top->signal_new_data.connect([this](std::vector<Channel> const& channels) {
        //m_main_window->graph_sc_top->
        size_t start_idx = channels[0].GetData().size() - static_cast<size_t>(m_main_window->graph_sc_bottom->GraphRegion().width);
        m_main_window->graph_sc_top->Draw()
    });

    m_dev_sc_bottom->signal_new_data.connect([this](std::vector<Channel> const& channels) {
        //m_main_window->graph_sc_bottom->
    });
}

Application::~Application()
{
    m_thread_info.join();
}