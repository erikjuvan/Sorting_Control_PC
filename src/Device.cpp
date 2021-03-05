#include "Device.hpp"
#include <iostream>
#include <map>

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

Device::Device(int id) :
    m_id(id)
{
    m_signals.resize(N_CHANNELS);

    Connect();
}