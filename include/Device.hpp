#pragma once

#include "Communication.hpp"
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

class Signal;
class Communication;

class Device
{
public:
    Device(int id);

private:
    void Connect();

    Communication       m_communication;
    std::vector<Signal> m_signals;

    ProtocolDataType m_data[N_CHANNELS * DATA_PER_CHANNEL];

    int         m_id{-1};
    std::string m_com_port;

    int m_sample_freq_hz;

    int m_rcv_packet_id              = 0; // it should be atomic but it is not neccessary since it's just informative counter
    int m_time_took_to_read_data_us  = 0;
    int m_time_took_to_parse_data_us = 0;
    int m_comm_speed_kb_s            = 0;
    int m_available_bytes            = 0;
};