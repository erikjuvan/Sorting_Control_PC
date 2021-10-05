#include "Device.hpp"

#include <future>
#include <iostream>
#include <chrono>
#include <map>

Device::Device(int id) :
    m_id(id)
{
    m_channels.resize(N_CHANNELS);

    Connect();
}

void Device::Connect()
{
    // Find only free ports
    auto all_ports  = m_communication.ListAllPorts();
    auto free_ports = m_communication.ListFreePorts();
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
        return;
    }

    std::map<int, std::pair<std::string, std::string>> port_map;

    for (auto const& [p, desc, hw_id] : ports) {
        if (m_communication.Connect(p)) {
            m_communication.StopTransmissionAndSuperPurge();
            auto tok = m_communication.WriteAndTokenizeResult("ID_G\n");
            if (tok.size() == 1)
                port_map[std::stoi(tok[0])] = std::make_pair(p, desc);

            m_communication.Disconnect();
        }
    }

    for (auto const& [id, pair] : port_map) {
        if (id == m_id) {
            std::string port_name = pair.first;

            if (m_communication.Connect(port_name))
                m_com_port = port_name;

            break;
        }
    }
}

void Device::GetData()
{    
    Header header = { 0, 0 };

    m_available_bytes = m_communication.GetRxBufferLen();
    
    // Find start of new packet by searching for the delimiter
    bool delimiter_found = false;
    do {
        m_communication.Read(&header.delim, sizeof(header.delim));
        
        if (header.delim == 0xDEADBEEF) {
            delimiter_found = true;
            break;
        }
    } while (m_communication.GetRxBufferLen());

    if (!delimiter_found) return;

    // Read packet ID
    auto len = m_communication.Read(&header.packet_id, sizeof(header.packet_id));

    // Check received data for valid size
    if (len != sizeof(header.packet_id)) {
        std::cerr << "Can't extract packet_id! Insufficient number of bytes read: " << len << " vs " << sizeof(header.packet_id) << "\n";
        return;
    }

    // Check header for valid packet ID
    if (header.packet_id != (m_prev_packet_id + 1)) { // if we missed a packet
        std::cerr << "Packet(s) lost: Should receive: " << m_prev_packet_id + 1 << " received: " << header.packet_id << ". Info: previous packet took: " << m_time_took_to_read_data_us / 1000 << " ms to read." << std::endl;
    }

    // Remember latest packet ID
    m_prev_packet_id = header.packet_id;
    m_rcv_packet_id = header.packet_id;

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<ProtocolDataType> data(N_CHANNELS * DATA_PER_CHANNEL);

    len = m_communication.Read(data.data(), data.size());

    if (len == data.size()) {
        for (size_t i = 0; i < m_channels.size(); ++i) {
            m_channels[i].AppendData(data.begin() + i * DATA_PER_CHANNEL, DATA_PER_CHANNEL);
        }
        signal_new_data(m_channels);
    }
    else {
        std::cerr << "Read insufficent bytes. Read " << len << " insted " << sizeof(data) << " bytes\n";
    }

    auto finished = std::chrono::high_resolution_clock::now();
    m_time_took_to_read_data_us = std::chrono::duration_cast<std::chrono::microseconds>(finished - start).count();
    m_comm_speed_kb_s = (len * 1000) / m_time_took_to_read_data_us; // kB/s
}