#pragma once

#include <string>

enum class Running { STOPPED,
                     RUNNING };
enum class Record { NO,
                    ALL,
                    EVENTS };
enum class View { RAW,
                  TRAINED,
                  FILTERED };
enum class TriggerFrame { OFF,
                          ON };

static constexpr int DATA_PER_CHANNEL{100};
static constexpr int N_CHANNELS{8};
static constexpr int ANALYSIS_PACKETS{10};

class Application
{
private:
    static void InitFromFile(const std::string& file_name);

public:
    static void Init();
    static void Run();

    static inline std::string config_com_port;
    static inline int         config_number_of_samples{10000}; // 10k
};