#include "Signal.hpp"
#include "Helpers.hpp"

// Static
/////////

void Signal::EventsToRecord(Event const events)
{
    events_to_record = events;
}

Signal::Event Signal::EventsToRecord()
{
    return events_to_record;
}

/////////

Signal::Signal() {}

Signal::Signal(int n, sf::Color col, const sf::FloatRect& region, float* max_val) :
    m_curve(sf::PrimitiveType::LineStrip, n),
    m_trigger_frame(sf::PrimitiveType::Lines, N_TRIGGER_FRAME_POINTS),
    m_event_indicator(sf::PrimitiveType::Lines, N_INDICATOR_POINTS),
    m_draw_trigger_frame(false), m_graph_region(region), m_max_val(max_val),
    m_events(Event::NONE)
{
    m_raw_data.reserve(10000 * 15 * 60); // reserve larger chunk of memory to avoid too many malloc/realloc's

    for (int i = 0; i < n; ++i) {
        m_curve[i].color      = col;
        m_curve[i].position.x = static_cast<float>(region.left + i * region.width / (n - 1));
    }

    // Trigger Frame
    for (int i = 0; i < N_TRIGGER_FRAME_POINTS; ++i)
        m_trigger_frame[i].color = col;
}

void Signal::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (m_draw) {
        if (m_draw_trigger_frame) {
            if (m_draw_event_indicator)
                target.draw(m_event_indicator); // draw first
            target.draw(m_trigger_frame);       // draw second
        }
        target.draw(m_curve); // draw third
    }
}

void Signal::SetThreashold(float threashold)
{
    m_threashold_value = threashold;
}

void Signal::SetBlindTime(int blind_time_value)
{
    m_blind_time_value = blind_time_value;
}

void Signal::EnableDraw()
{
    m_draw = true;
}

void Signal::DisableDraw()
{
    m_draw = false;
}

void Signal::EnableTriggerFrame()
{
    m_draw_trigger_frame = true;
}

void Signal::DisableTriggerFrame()
{
    m_draw_trigger_frame = false;
}

void Signal::ShowEventIndicator(bool show)
{
    m_draw_event_indicator = show;
}

void Signal::OnlyDrawOnTrigger(bool on)
{
    if (on)
        m_only_draw_on_trigger = true;
    else
        m_only_draw_on_trigger = false;
}

int Signal::GetDetectionsInWindow() const
{
    return m_detected_in_window_cnt;
}
void Signal::ClearDetectionsInWindow()
{
    m_detected_in_window_cnt = 0;
}

int Signal::GetDetectionsOutWindow() const
{
    return m_detected_out_window_cnt;
}
void Signal::ClearDetectionsOutWindow()
{
    m_detected_out_window_cnt = 0;
}

int Signal::GetMissed() const
{
    return m_detection_missed;
}
void Signal::ClearMissed()
{
    m_detection_missed = 0;
}

void Signal::SetColor(sf::Color const& col)
{
    size_t const count = m_curve.getVertexCount();
    for (int i = 0; i < count; ++i)
        m_curve[i].color = col;
}

bool Signal::AnyEvents() const
{
    return (events_to_record & m_events) != 0;
}

void Signal::ClearEvents()
{
    m_events = Event::NONE;
}

void Signal::SetIndicator(float const x, Event const ev)
{
    const float y_zero = m_graph_region.top + m_graph_region.height;
    const float y_high = y_zero - (m_threashold_value / *m_max_val) * m_graph_region.height + 1;

    auto lambda_set_indicator = [this, x, y_zero, y_high](sf::Color const& col) {
        m_event_indicator[m_event_indicator_idx].color      = col;
        m_event_indicator[m_event_indicator_idx++].position = sf::Vector2f(x, y_zero + 20); // make it visible at the bottom
        m_event_indicator[m_event_indicator_idx].color      = col;
        m_event_indicator[m_event_indicator_idx++].position = sf::Vector2f(x, y_zero - m_graph_region.height / 2);
    };

    constexpr int alpha = 100;
    switch (ev) {
    case Event::MISSED:
        if (Signal::EventsToRecord() & Event::MISSED) {
            lambda_set_indicator(sf::Color(255, 0, 0, alpha));
        }
        break;
    case Event::DETECTED_OUT:
        if (Signal::EventsToRecord() & Event::DETECTED_OUT) {
            lambda_set_indicator(sf::Color(0, 0, 255, alpha));
        }
        break;
    case Event::DETECTED_IN:
        if (Signal::EventsToRecord() & Event::DETECTED_IN) {
            lambda_set_indicator(sf::Color(0, 255, 0, alpha));
        }
        break;
    case Event::DETECTION_TIME:
        if (Signal::EventsToRecord() & Event::DETECTION_TIME) {
            lambda_set_indicator(sf::Color(255, 0, 255, alpha));
        }
        break;
    case Event::WINDOW_TIME:
        if (Signal::EventsToRecord() & Event::WINDOW_TIME) {
            lambda_set_indicator(sf::Color(0, 255, 255, alpha));
        }
        break;
    }
}

// Return false if a signal never reached the threashold value when the window was on
void Signal::Edit(const float* buf, int start, int size)
{
    // union used to make conversion from int<->float conform to standard (strict aliasing)
    union float_int {
        uint32_t i;
        float    f;
    };

    const float y_zero = m_graph_region.top + m_graph_region.height;
    const float y_high = y_zero - (m_threashold_value / *m_max_val) * m_graph_region.height + 1;

    if (m_draw_trigger_frame) {

        if (start == 0) {
            // Clear all frames
            for (int i = 0; i <= m_trigger_frame_idx; ++i)
                m_trigger_frame[i].position = sf::Vector2f(0.f, 0.f);
            m_trigger_frame_idx = 0;

            for (int i = 0; i <= m_event_indicator_idx; ++i)
                m_event_indicator[i].position = sf::Vector2f(0.f, 0.f);
            m_event_indicator_idx = 0;

            if (m_only_draw_on_trigger)
                DisableDraw();
            else
                EnableDraw();

            ClearEvents();
        }

        for (int i = 0, s = start; i < size; ++i, ++s) {
            float_int sig_val_conv;
            sig_val_conv.f = buf[i];

            // Edge detection
            /////////////////
            bool trigger_active   = sig_val_conv.i & (1 << 31); // check MSB bit
            m_diff                = trigger_active - m_trigger_active_prev;
            m_trigger_active_prev = trigger_active;
            /////////////////

            // Value correction
            sig_val_conv.i = sig_val_conv.i & 0x7FFFFFFF;
            float sig_val  = sig_val_conv.f;

            // Threashold detection
            ///////////////////////
            m_blind_time -= (m_blind_time > 0);
            if (sig_val >= m_threashold_value && m_blind_time <= 0) {
                m_threashold = Threashold::REACHED;
                m_blind_time = m_blind_time_value;
            }
            ///////////////////////

            const float x_position = m_curve[s].position.x;

            if (m_diff == 1) { // rising edge
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_zero);
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_high);
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_high);

                if (m_only_draw_on_trigger)
                    EnableDraw();
                m_threashold = Threashold::SEARCHING;
                m_detection_stats.Reset();
                m_trigger_window_stats.Reset();
            } else if (m_diff == -1) { // falling edge
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_high);
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_high);
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_zero);

                if (m_threashold == Threashold::SEARCHING) {
                    m_threashold = Threashold::MISSED;
                    m_detection_missed++;
                    m_events = static_cast<Event>(m_events | Event::MISSED);
                    SetIndicator(x_position, Event::MISSED);
                }

                if (m_threashold == Threashold::REACHED) { // Special case where we reached threashold on falling edge
                    m_threashold = Threashold::MISSED;
                    m_detection_missed++;
                    m_detected_out_window_cnt++;
                    m_events = static_cast<Event>(m_events | Event::MISSED | Event::DETECTED_OUT);
                    SetIndicator(x_position, Event::MISSED);
                    SetIndicator(x_position, Event::DETECTED_OUT);
                }

                m_trigger_window_stats.Update(&Signal::trigger_window_stats_all);
                if (Signal::window_time_min > m_trigger_window_stats.Get().last || m_trigger_window_stats.Get().last > Signal::window_time_max) {
                    m_events = static_cast<Event>(m_events | Event::WINDOW_TIME);
                    SetIndicator(x_position, Event::WINDOW_TIME);
                }
            } else if (trigger_active) { // frame active
                if (s == 0) {            // new start
                    m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[0].position.x, y_high);
                }
                m_trigger_frame[m_trigger_frame_idx].position = sf::Vector2f(x_position, y_high);

                m_detection_stats.Increment();
                m_trigger_window_stats.Increment();
                if (m_threashold == Threashold::REACHED) {
                    m_detected_in_window_cnt++;
                    m_threashold = Threashold::IDLE;
                    m_events     = static_cast<Event>(m_events | Event::DETECTED_IN);
                    m_detection_stats.Update(&Signal::detection_stats_all);
                    SetIndicator(x_position, Event::DETECTED_IN);
                    if (Signal::detection_time_min > m_detection_stats.Get().last || m_detection_stats.Get().last > Signal::detection_time_max) {
                        m_events = static_cast<Event>(m_events | Event::DETECTION_TIME);
                        SetIndicator(x_position, Event::DETECTION_TIME);
                    }
                }
            } else { // no frame
                if (m_threashold == Threashold::REACHED) {
                    m_detected_out_window_cnt++;
                    m_threashold = Threashold::IDLE;
                    m_events     = static_cast<Event>(m_events | Event::DETECTED_OUT);
                    SetIndicator(x_position, Event::DETECTED_OUT);
                }
            }

            // Added to speed up in case we draw frame (so we don't need to iterate throught it again)
            m_curve[s].position.y = y_zero - (sig_val / *m_max_val) * m_graph_region.height + 1;
            m_raw_data.push_back(buf[i]);
        }
    } else {
        for (int i = 0, s = start; i < size; ++i, ++s) {
            float_int sig_val_conv;
            sig_val_conv.f        = buf[i];
            sig_val_conv.i        = sig_val_conv.i & 0x7FFFFFFF;
            float sig_val         = sig_val_conv.f;
            m_curve[s].position.y = y_zero - (sig_val / *m_max_val) * m_graph_region.height + 1;
            m_raw_data.push_back(buf[i]);

            /* Not implemented because it is not neccessary and just waists cpu
			if (m_curve[s].position.y < m_graph_region.top) { // curve out of graph region
			m_curve[s].position.y = m_graph_region.top;
			}
			*/
        }
    }
}
