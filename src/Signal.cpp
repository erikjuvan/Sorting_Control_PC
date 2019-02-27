#include "Signal.hpp"
#include "Helpers.hpp"

Signal::Signal() {}

Signal::Signal(int n, sf::Color col, const sf::FloatRect& region, std::shared_ptr<float const> const& max_val) :
    m_curve(sf::PrimitiveType::LineStrip, n),
    m_trigger_frame(sf::PrimitiveType::Lines, N_TRIGGER_FRAME_POINTS),
    m_event_indicator(sf::PrimitiveType::Lines, N_INDICATOR_POINTS),
    m_graph_region(region), m_max_val(max_val),
    m_events(Event::NONE)
{
    m_rx_data.reserve(10000 * 10 * 60); // reserve larger chunk of memory to avoid too many malloc/realloc's

    for (int i = 0; i < n; ++i) {
        m_curve[i].color      = col;
        m_curve[i].position.x = static_cast<float>(region.left + i * region.width / (n - 1));
    }

    // Trigger Frame
    for (int i = 0; i < N_TRIGGER_FRAME_POINTS; ++i)
        m_trigger_frame[i].color = col;

    m_ejection_window_stats = std::make_shared<Statistics<int64_t>>();
    m_detection_stats       = std::make_shared<Statistics<int64_t>>();
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

void Signal::EventsToRecord(std::shared_ptr<Event const> events_to_record)
{
    m_events_to_record = events_to_record;
}

void Signal::WindowAndDetectionTimeLimits(
    std::shared_ptr<uint32_t const> detection_time_min,
    std::shared_ptr<uint32_t const> detection_time_max,
    std::shared_ptr<uint32_t const> window_time_min,
    std::shared_ptr<uint32_t const> window_time_max)
{
    m_detection_time_min = detection_time_min;
    m_detection_time_max = detection_time_max;
    m_window_time_min    = window_time_min;
    m_window_time_max    = window_time_max;
}

void Signal::SetThreashold(float threashold)
{
    m_threashold_value = threashold;
}

void Signal::SetBlindTicks(int blind_ticks)
{
    m_blind_ticks_param = blind_ticks;
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
    return (*m_events_to_record & m_events) != 0;
}

void Signal::ClearEvents()
{
    m_events = Event::NONE;
}

void Signal::SetIndicator(float const x, Event const ev)
{
    // Check if we are trying to draw too many indicators and reset
    if (m_event_indicator_idx + 1 >= m_event_indicator.getVertexCount())
        m_event_indicator_idx = 0;

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
        if (*m_events_to_record & Event::MISSED) {
            lambda_set_indicator(sf::Color(255, 0, 0, alpha));
        }
        break;
    case Event::DETECTED_OUT:
        if (*m_events_to_record & Event::DETECTED_OUT) {
            lambda_set_indicator(sf::Color(0, 0, 255, alpha));
        }
        break;
    case Event::DETECTED_IN:
        if (*m_events_to_record & Event::DETECTED_IN) {
            lambda_set_indicator(sf::Color(0, 255, 0, alpha));
        }
        break;
    case Event::DETECTION_TIME:
        if (*m_events_to_record & Event::DETECTION_TIME) {
            lambda_set_indicator(sf::Color(255, 0, 255, alpha));
        }
        break;
    case Event::WINDOW_TIME:
        if (*m_events_to_record & Event::WINDOW_TIME) {
            lambda_set_indicator(sf::Color(0, 255, 255, alpha));
        }
        break;
    }
}

// Return false if a signal never reached the threashold value when the window was on
void Signal::Edit(ProtocolDataType const* m_data, int start, int size, View view)
{
    const float y_zero = m_graph_region.top + m_graph_region.height;
    const float y_high = y_zero - (m_threashold_value / *m_max_val) * m_graph_region.height + 1;

    for (int i = 0, s = start; i < size; ++i, ++s) {
        m_rx_data.push_back(m_data[i].u64);

        uint32_t raw_data     = m_data[i].u32.raw_data;
        bool     ejection_win = m_data[i].u32.ejection_window;
        float    filt_data    = std::abs(m_data[i].f32.filtered_data_w_obj_det); // we have to take the absolute value (obj_det is on MSB)
        bool     obj_det      = m_data[i].u32.object_detected;

        if (view == View::RAW)
            m_curve[s].position.y = y_zero - (raw_data / *m_max_val) * m_graph_region.height + 1;
        else if (view == View::FILTERED)
            m_curve[s].position.y = y_zero - (filt_data / *m_max_val) * m_graph_region.height + 1;

        if (m_draw_trigger_frame) {

            if (start == 0) {
                // Clear all frames
                for (int i = 0; i < m_trigger_frame_idx; ++i)
                    m_trigger_frame[i].position = sf::Vector2f(0.f, 0.f);
                m_trigger_frame_idx = 0;

                // Clear all indicators
                for (int i = 0; i < m_event_indicator_idx; ++i)
                    m_event_indicator[i].position = sf::Vector2f(0.f, 0.f);
                m_event_indicator_idx = 0;

                if (m_only_draw_on_trigger)
                    DisableDraw();
                else
                    EnableDraw();

                ClearEvents();
            }

            // Edge detection
            /////////////////
            m_diff              = ejection_win - m_ejection_win_prev;
            m_ejection_win_prev = ejection_win;
            /////////////////

            // Threshold detection
            ///////////////////////
            m_blind_ticks -= (m_blind_ticks > 0);
            if (filt_data >= m_threashold_value && m_blind_ticks <= 0) {
                m_threshold   = Threshold::REACHED;
                m_blind_ticks = m_blind_ticks_param;
            }
            ///////////////////////

            const float x_position = m_curve[s].position.x;

            if (m_diff == 1) { // rising edge
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_zero);
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_high);
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_high);

                if (m_only_draw_on_trigger)
                    EnableDraw();
                m_threshold                  = Threshold::SEARCHING;
                m_ejection_window_width_cntr = 0;
                m_detection_time_cntr        = 0;
            } else if (m_diff == -1) { // falling edge
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_high);
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_high);
                m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(x_position, y_zero);

                if (m_threshold == Threshold::SEARCHING) {
                    m_threshold = Threshold::MISSED;
                    m_detection_missed++;
                    m_events = static_cast<Event>(m_events | Event::MISSED);
                    SetIndicator(x_position, Event::MISSED);
                }

                if (m_threshold == Threshold::REACHED) { // Special case where we reached threashold on falling edge
                    m_threshold = Threshold::MISSED;
                    m_detection_missed++;
                    m_detected_out_window_cnt++;
                    m_events = static_cast<Event>(m_events | Event::MISSED | Event::DETECTED_OUT);
                    SetIndicator(x_position, Event::MISSED);
                    SetIndicator(x_position, Event::DETECTED_OUT);
                }

                m_ejection_window_stats->Update(m_ejection_window_width_cntr / (m_sample_freq_hz / 1000.f)); // convert to milliseconds
                if (m_ejection_window_stats->last < *m_window_time_min || m_ejection_window_stats->last > *m_window_time_max) {
                    m_events = static_cast<Event>(m_events | Event::WINDOW_TIME);
                    SetIndicator(x_position, Event::WINDOW_TIME);
                }
            } else if (ejection_win) { // frame active
                if (s == 0) {          // new start
                    m_trigger_frame[m_trigger_frame_idx++].position = sf::Vector2f(m_curve[0].position.x, y_high);
                }
                m_trigger_frame[m_trigger_frame_idx].position = sf::Vector2f(x_position, y_high);

                // Increment window width and detection time counters
                m_ejection_window_width_cntr++;
                m_detection_time_cntr++;

                if (m_threshold == Threshold::REACHED) {
                    m_detected_in_window_cnt++;
                    m_threshold = Threshold::IDLE;
                    m_events    = static_cast<Event>(m_events | Event::DETECTED_IN);
                    m_detection_stats->Update(m_detection_time_cntr / (m_sample_freq_hz / 1000.f)); // convert to milliseconds
                    SetIndicator(x_position, Event::DETECTED_IN);
                    if (m_detection_stats->last < *m_detection_time_min || m_detection_stats->last > *m_detection_time_max) {
                        m_events = static_cast<Event>(m_events | Event::DETECTION_TIME);
                        SetIndicator(x_position, Event::DETECTION_TIME);
                    }
                }
            } else { // no frame
                if (m_threshold == Threshold::REACHED) {
                    m_detected_out_window_cnt++;
                    m_threshold = Threshold::IDLE;
                    m_events    = static_cast<Event>(m_events | Event::DETECTED_OUT);
                    SetIndicator(x_position, Event::DETECTED_OUT);
                }
            }
        }
    }
}
