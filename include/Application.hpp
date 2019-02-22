#pragma once

#include <chrono>
#include <optional>
#include <string>

enum class Running { STOPPED,
                     RUNNING };
enum class Record { NO,
                    ALL,
                    EVENTS };
enum class View { RAW,
                  FILTERED };
enum class TriggerFrame { OFF,
                          ON };

static constexpr int DATA_PER_CHANNEL{100};
static constexpr int N_CHANNELS{8};
static constexpr int ANALYSIS_PACKETS{10};

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
    uint32_t delim;
    uint32_t packet_id;
};

class Application
{
private:
    static void InitFromFile(const std::string& file_name);

public:
    static void Init();
    static void Run();

    static inline std::string config_com_port;
    static inline int         config_number_of_samples{10000}; // 10k

    static inline std::chrono::time_point<std::chrono::steady_clock> run_start_time;
};