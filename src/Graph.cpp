#include "Graph.hpp"
#include <iomanip>
#include <mygui/ResourceManager.hpp>
#include <sstream>

Graph::Graph(int x, int y, int w, int h, float max_val) :
    m_max_val(std::make_shared<float>(max_val)), m_background(sf::Vector2f(w, h)),
    m_graph_region(sf::Vector2f(w - 5 * m_margin, h - 3 * m_margin))
{
    m_background.setPosition(x, y);
    m_background.setOutlineColor(sf::Color::Black);
    m_background.setOutlineThickness(1.f);

    m_num_of_points = m_graph_region.getSize().x;

    m_graph_region.setPosition(x + 3.f * m_margin, y + 1.f * m_margin);
    m_graph_region.setOutlineColor(sf::Color::Black);
    m_graph_region.setOutlineThickness(1.f);
    m_graph_rect = m_graph_region.getGlobalBounds();

    m_font.loadFromFile(mygui::ResourceManager::GetSystemFontName());

    m_x_axis.setFont(m_font);
    m_x_axis.setFillColor(sf::Color::Black);
    m_x_axis.setCharacterSize(16);
    m_x_axis.setString("Sample");
    m_x_axis.setPosition(sf::Vector2f(x + w / 2.f - m_x_axis.getLocalBounds().width / 2.f, y - h + 1.25f * m_margin));
}

void Graph::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_background, states);
    target.draw(m_graph_region, states);
    target.draw(m_x_axis);
    target.draw(m_grid);
    for (auto& m : m_x_axis_markers)
        target.draw(m);
    for (auto& m : m_y_axis_markers)
        target.draw(m);
    for (int i = 0; i < m_signals.size(); ++i) {
        if (m_draw_signal[i])
            target.draw(*m_signals[i]);
    }
}

void Graph::Handle(const sf::Event& event)
{
    if (!Enabled())
        return;

    //  && m_graph_region.getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x, event.mouseButton.y))
    if (event.type == sf::Event::MouseWheelScrolled && m_mouseover) {
        if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
            if (*m_max_val >= 100.f) {
                *m_max_val -= event.mouseWheelScroll.delta * 50.f;
            } else if (*m_max_val > 5.f) {
                *m_max_val -= event.mouseWheelScroll.delta * 5.f;
            } else {
                if (event.mouseWheelScroll.delta < 0.f)
                    *m_max_val -= event.mouseWheelScroll.delta * 5.f;
            }

            CreateAxisMarkers();
        }
    } else if (event.type == sf::Event::KeyReleased && m_mouseover) {
        m_onKeyPress(event);
    } else if (event.type == sf::Event::MouseMoved) {
        if (m_graph_region.getGlobalBounds().contains(sf::Vector2f(event.mouseMove.x, event.mouseMove.y))) {
            m_mouseover = true;
        } else {
            m_mouseover = false;
        }
    }
}

void Graph::Enabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled) {
    } else {
    }
}

bool Graph::Enabled() const
{
    return m_enabled;
}

void Graph::AddSignal(std::shared_ptr<Signal> const& signal)
{
    m_signals.push_back(signal);
    m_draw_signal.push_back(true);
}

void Graph::ChangeSignal(int idx, std::shared_ptr<Signal> const& signal)
{
    if (idx < m_signals.size()) {
        m_signals[idx] = signal;
    }
}

// n_lines - number of one type of lines (vertical or horizontal), there are same number of other lines
void Graph::CreateGrid(int n_lines)
{
    m_num_grid_lines      = n_lines;
    m_grid                = sf::VertexArray(sf::PrimitiveType::Lines, n_lines * 4);
    const sf::Color color = sf::Color(100, 100, 100, 65);

    // vertical lines
    for (int i = 0; i < n_lines; ++i) {
        m_grid[2 * i].position     = sf::Vector2f(m_graph_rect.left + (i + 1) * (m_graph_rect.width / (n_lines + 1)), m_graph_rect.top);
        m_grid[2 * i].color        = color;
        m_grid[2 * i + 1].position = sf::Vector2f(m_graph_rect.left + (i + 1) * (m_graph_rect.width / (n_lines + 1)), m_graph_rect.top + m_graph_rect.height);
        m_grid[2 * i + 1].color    = color;
    }

    // horizontal lines
    for (int i = n_lines, j = 0; i < n_lines * 2; ++i, ++j) {
        m_grid[2 * i].position     = sf::Vector2f(m_graph_rect.left, m_graph_rect.top + ((j + 1) * (m_graph_rect.height / (n_lines + 1))));
        m_grid[2 * i].color        = color;
        m_grid[2 * i + 1].position = sf::Vector2f(m_graph_rect.left + m_graph_rect.width, m_graph_rect.top + ((j + 1) * (m_graph_rect.height / (n_lines + 1))));
        m_grid[2 * i + 1].color    = color;
    }

    CreateAxisMarkers();
}

void Graph::CreateAxisMarkers()
{
    sf::FloatRect rect = m_graph_region.getGlobalBounds();
    int           n    = m_num_grid_lines + 2; // + 2 is for 0 and max

    m_x_axis_markers.clear();
    m_y_axis_markers.clear();

    // X
    m_x_axis_markers.reserve(n);
    for (int i = 0; i < n; ++i) {
        m_x_axis_markers.push_back(sf::Text());
        auto& marker = m_x_axis_markers[m_x_axis_markers.size() - 1];
        marker.setFont(m_font);
        marker.setFillColor(sf::Color::Black);
        marker.setCharacterSize(18);
        int tmp = i * m_num_of_points / (n - 1);
        marker.setString(std::to_string(tmp));
        marker.setOrigin(marker.getLocalBounds().left + marker.getLocalBounds().width / 2.f,
                         marker.getLocalBounds().top + marker.getLocalBounds().height / 2.f);
        marker.setPosition(rect.left + i * rect.width / (n - 1), rect.top + rect.height + marker.getLocalBounds().height);
    }

    // Y
    m_y_axis_markers.reserve(n);
    for (int i = 0; i < n; ++i) {
        m_y_axis_markers.push_back(sf::Text());
        auto& marker = m_y_axis_markers[m_y_axis_markers.size() - 1];
        marker.setFont(m_font);
        marker.setFillColor(sf::Color::Black);
        marker.setCharacterSize(18);
        float             tmp = i * *m_max_val / (n - 1);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << tmp;
        marker.setString(ss.str());
        marker.setOrigin(marker.getLocalBounds().left + marker.getLocalBounds().width / 2.f,
                         marker.getLocalBounds().top + marker.getLocalBounds().height / 2.f);
        marker.setPosition(rect.left - marker.getLocalBounds().width / 2 - 3, rect.top + rect.height - i * rect.height / (n - 1));
    }
}

const sf::FloatRect& Graph::GraphRegion()
{
    return m_graph_rect;
}

std::shared_ptr<float const> Graph::MaxVal()
{
    return m_max_val;
}

void Graph::EnableTriggerFrame()
{
    for (const auto& s : m_signals)
        s->EnableTriggerFrame();
}

void Graph::DisableTriggerFrame()
{
    for (const auto& s : m_signals)
        s->DisableTriggerFrame();
}

void Graph::OnKeyPress(const graph_callback_type& f)
{
    m_onKeyPress = f;
}

void Graph::ToggleDrawSignal(int idx)
{
    if (idx > 0 && idx <= m_signals.size())
        m_draw_signal[idx - 1] = !m_draw_signal[idx - 1];
}

void Graph::ToggleDrawAllSignals()
{
    m_draw_all_signals = !m_draw_all_signals;
    std::fill(m_draw_signal.begin(), m_draw_signal.end(), m_draw_all_signals);
}

void Graph::UpdateSignals(ProtocolDataType* data)
{
    // Update signals with new m_data
    for (auto& s : signals) {
        s->Edit(data, m_signal_update_cntr * DATA_PER_CHANNEL, DATA_PER_CHANNEL, m_view);
        data += DATA_PER_CHANNEL;
    }

    // If we filled up char/signal
    if (++m_signal_update_cntr >= (m_config_number_of_samples / DATA_PER_CHANNEL)) {
        m_signal_update_cntr = 0;
        if (*m_record == Record::ALL) {
            for (auto const& s : signals)
                recorded_signals.emplace_back(std::make_shared<Signal>(*s));
            label_recorded_signals_counter->SetText(std::to_string(recorded_signals.size() / N_CHANNELS));
        }
        else if (*m_record == Record::EVENTS) {
            bool event_happened = false;
            for (auto const& s : signals) {
                if (s->AnyEvents()) {
                    event_happened = true;
                    break;
                }
            }

            if (event_happened) {
                for (auto& s : signals) {
                    if (s->AnyEvents()) {
                        recorded_signals.push_back(std::make_shared<Signal>(*s));
                        s->ClearEvents();
                    }
                    else {
                        recorded_signals.push_back(std::make_shared<Signal>()); // push empty signal
                    }
                }
                label_recorded_signals_counter->SetText(std::to_string(recorded_signals.size() / N_CHANNELS));
            }
        }

        if (m_info_win_frm_sc_top)
            m_info_win_frm_sc_top->RefreshTable();
        if (m_info_win_det_sc_top)
            m_info_win_det_sc_top->RefreshTable();
    }
}