#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mygui/ResourceManager.hpp>
#include <optional>
#include <string>
#include <thread>

enum class Record { NO,
                    ALL,
                    EVENTS };
enum class View { RAW,
                  FILTERED };

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

class MainWindow;
class Communication;
class InfoWindow;

class Application
{
private:
    // Members
    std::unique_ptr<MainWindow>    m_mainWindow;
    std::shared_ptr<Communication> m_communication;
    std::shared_ptr<InfoWindow>    m_detectionInfoWindow;
    std::shared_ptr<InfoWindow>    m_frameInfoWindow;

    std::shared_ptr<mygui::ResourceManager> m_rm;

    std::shared_ptr<bool>   m_running;
    std::shared_ptr<Record> m_record;

    std::string m_config_com_port;
    int         m_config_number_of_samples{10000}; // 10k

    std::thread m_thread_info;
    std::thread m_thread_get_data;

    int m_rcv_packet_id              = 0; // it should be atomic but it is not neccessary since it's just informative counter
    int m_time_took_to_read_data_us  = 0;
    int m_time_took_to_parse_data_us = 0;
    int m_comm_speed_kb_s            = 0;
    int m_available_bytes            = 0;

    ProtocolDataType m_data[N_CHANNELS * DATA_PER_CHANNEL];

    // Methods
    void InitFromFile(const std::string& file_name);
    void Information();
    void GetData();

public:
    Application();
    ~Application();

    void Run();
};