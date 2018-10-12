#pragma once

#include "Helpers.hpp"
#include <SFML/Graphics.hpp>

class Signal : public sf::Drawable
{
private:
    enum class Threashold { IDLE,
                            REACHED,
                            MISSED,
                            SEARCHING };

    class SignalStats
    {
    private:
        Statistics<int64_t> m_stats;
        int                 m_val     = 0;
        bool                m_updated = false;

    public:
        inline void Increment() { m_val++; }

        void Reset()
        {
            m_val     = 0;
            m_updated = false;
        }

        void Update(Statistics<int64_t>* ext_stat)
        {
            if (!m_updated) {
                m_stats.Update(m_val);
                m_stats.push_back(m_val);
                if (ext_stat) {
                    ext_stat->Update(m_val);
                    ext_stat->push_back(m_val);
                }
                m_updated = true;
            }
        }

        auto& Get() { return m_stats; }
    };

public:
    enum Event { NONE         = 0x0,
                 DETECTED_IN  = 0x1,
                 DETECTED_OUT = 0x2,
                 MISSED       = 0x4 };

    static void  EventsToRecord(Event const events);
    static Event EventsToRecord();
    static auto* GetTriggerWindowStatsAll() { return &m_trigger_window_stats_all; }
    static auto* GetDetecionStatsAll() { return &m_detection_stats_all; }

    Signal();
    Signal(int n, sf::Color col, const sf::FloatRect& region, float* max_val);

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    void  SetThreashold(float threashold);
    void  SetBlindTime(int blind_time_value);
    void  EnableTriggerFrame();
    void  DisableTriggerFrame();
    void  EnableDraw();
    void  DisableDraw();
    void  OnlyDrawOnTrigger(bool on);
    int   GetDetectionsInWindow() const;
    void  ClearDetectionsInWindow();
    int   GetDetectionsOutWindow() const;
    void  ClearDetectionsOutWindow();
    int   GetMissed() const;
    void  ClearMissed();
    void  SetColor(sf::Color const& col);
    bool  AnyEvents() const;
    void  ClearEvents();
    void  Edit(float* buf, int start, int size); // Return false if a signal never reached the threashold value when the window was on
    auto& GetTriggerWindowStats() { return m_trigger_window_stats.Get(); }
    auto& GetDetecionStats() { return m_detection_stats.Get(); }

private:
    static constexpr int N_TRIGGER_FRAME_POINTS = 60; // should be enough for ~ 60 / 3 = 20 windows

    inline static Event               m_events_to_record;
    inline static Statistics<int64_t> m_trigger_window_stats_all;
    inline static Statistics<int64_t> m_detection_stats_all;

    Threashold m_threashold;
    Event      m_events;

    SignalStats m_trigger_window_stats;
    SignalStats m_detection_stats;

    sf::VertexArray m_curve;
    sf::VertexArray m_trigger_frame;
    int             m_trigger_frame_idx{0};
    sf::FloatRect   m_graph_region;

    bool m_draw{true};
    bool m_only_draw_on_trigger{false};

    float* m_max_val;
    float  m_threashold_value;
    bool   m_draw_trigger_frame{false};

    int m_diff{0};
    int m_trigger_val{0};
    int m_trigger_val_prev{0};

    int m_detected_in_window_cnt{0};
    int m_detected_out_window_cnt{0};
    int m_detection_missed{0};
    int m_blind_time{0};
    int m_blind_time_value;
};
