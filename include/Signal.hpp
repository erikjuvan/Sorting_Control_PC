#pragma once

#include "Application.hpp"
#include "Helpers.hpp"
#include <SFML/Graphics.hpp>

class Signal : public sf::Drawable
{
public:
    enum Event { NONE                  = 0x0,
                 DETECTED_IN           = 0x1,
                 DETECTED_OUT          = 0x2,
                 MISSED                = 0x4,
                 WINDOW_TIME           = 0x8,
                 DETECTION_TIME        = 0x10,
                 THRESHOLD_READ_BY_PLC = 0x20,
    };

    Signal();
    Signal(int n, sf::Color col, const sf::FloatRect& region, std::shared_ptr<float const> const& max_val);

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    void EventsToRecord(std::shared_ptr<Event const> events_to_record);
    void WindowAndDetectionTimeLimits(
        std::shared_ptr<uint32_t const> detection_time_min,
        std::shared_ptr<uint32_t const> detection_time_max,
        std::shared_ptr<uint32_t const> window_time_min,
        std::shared_ptr<uint32_t const> window_time_max);

    void        SetThreashold(float threashold);
    void        SetBlindTicks(int blind_ticks);
    void        EnableDraw();
    void        DisableDraw();
    void        EnableTriggerFrame();
    void        DisableTriggerFrame();
    void        ShowEventIndicator(bool show);
    void        OnlyDrawOnTrigger(bool on);
    int         GetDetectionsInWindow() const;
    void        ClearDetectionsInWindow();
    int         GetDetectionsOutWindow() const;
    void        ClearDetectionsOutWindow();
    int         GetMissed() const;
    void        ClearMissed();
    void        SetColor(sf::Color const& col);
    bool        AnyEvents() const;
    void        ClearEvents();
    void        Edit(ProtocolDataType const* m_data, int start, int size, View view); // Return false if a signal never reached the threashold value when the window was on
    const auto& GetRXData() const { return m_rx_data; }
    void        ClearRXData() { m_rx_data.clear(); }
    auto&       GetTriggerWindowStats() { return m_ejection_window_stats; }
    auto&       GetDetectionStats() { return m_detection_stats; }

private:
    enum class Threshold {
        IDLE,
        REACHED,
        MISSED,
        SEARCHING
    };

    void SetIndicator(float const x, Signal::Event const ev);

    const int N_TRIGGER_FRAME_POINTS = 600; // should be enough for ~ 600 / 6 = 100 windows
    const int N_INDICATOR_POINTS     = 200; // should be enough for ~ 200 / 2 = 100 indicators

    std::shared_ptr<uint32_t const> m_detection_time_min;
    std::shared_ptr<uint32_t const> m_detection_time_max;
    std::shared_ptr<uint32_t const> m_window_time_min;
    std::shared_ptr<uint32_t const> m_window_time_max;
    std::shared_ptr<Event const>    m_events_to_record;

    std::shared_ptr<Statistics<int64_t>> m_ejection_window_stats;
    std::shared_ptr<Statistics<int64_t>> m_detection_stats;
    uint32_t                             m_ejection_window_width_cntr = 0;
    uint32_t                             m_detection_time_cntr        = 0;

    Threshold m_threshold = Threshold::IDLE;
    Event     m_events    = Event::NONE;

    std::vector<uint64_t> m_rx_data;
    sf::VertexArray       m_curve;
    sf::VertexArray       m_trigger_frame;
    int                   m_trigger_frame_idx{0};
    sf::VertexArray       m_event_indicator;
    int                   m_event_indicator_idx{0};
    sf::FloatRect         m_graph_region;

    bool m_draw{true};
    bool m_only_draw_on_trigger{false};

    std::shared_ptr<float const> m_max_val;
    float                        m_threashold_value;
    bool                         m_draw_trigger_frame{true};
    bool                         m_draw_event_indicator{true};

    bool m_ejection_win_prev{false};
    bool m_obj_det_prev{false};

    int m_detected_in_window_cnt{0};
    int m_detected_out_window_cnt{0};
    int m_detection_missed{0};
    int m_blind_ticks{0};
    int m_blind_ticks_param;
};
