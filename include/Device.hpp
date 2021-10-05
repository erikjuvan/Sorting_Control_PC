#pragma once

#include "Communication.hpp"
#include "lsignal.hpp"
#include <vector>

constexpr int DATA_PER_CHANNEL{100};
constexpr int N_CHANNELS{8};

union ProtocolDataType {
    uint64_t u64;
    struct {
        uint32_t raw_data : 31;
        uint32_t ejection_window : 1;
        uint32_t undef : 31;
        uint32_t object_detected : 1;
    } u32;

    struct {
        float undef;
        float filtered_data_w_obj_det;
    } f32;
};

struct Header {
    uint32_t delim     = 0;
    uint32_t packet_id = 0;
};

class Channel {
public:

    Channel()
    {
        m_data.reserve(1e6); // just to prevent too many resizes
    }

    void AppendData(std::vector<ProtocolDataType> const& data)
    {
        m_data.insert(m_data.end(), data.begin(), data.end());
    }

    void AppendData(ProtocolDataType const* data, int size)
    {
        m_data.insert(m_data.end(), data, data + size);
    }

    void AppendData(std::vector<ProtocolDataType>::iterator const& it, int size)
    {
        m_data.insert(m_data.end(), it, it + size);
    }

    std::vector<ProtocolDataType> const& GetData() const
    {
        return m_data;
    }

    void Clear()
    {
        m_data.clear();
    }

private:
    std::vector<ProtocolDataType> m_data;
};

class Device
{
public:
    Device(int id);

    Channel const& GetChannel(int idx) const
    {
        return m_channels.at(idx);
    }

    lsignal::signal<void(std::vector<Channel> const&)> signal_new_data;

private:
    void Connect();
    void GetData();

    int m_id{ -1 };

    Communication        m_communication;
    std::vector<Channel> m_channels;
    
    std::string m_com_port;

    int m_sample_freq_hz;

    int m_rcv_packet_id              = 0;
    int m_prev_packet_id             = 0;
    int m_time_took_to_read_data_us  = 0;
    int m_time_took_to_parse_data_us = 0;
    int m_comm_speed_kb_s            = 0;
    int m_available_bytes            = 0;
};