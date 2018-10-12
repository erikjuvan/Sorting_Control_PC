#pragma once

#include "Signal.hpp"
#include <mygui/Object.hpp>

class Chart : public mygui::Object
{
private:
    using chart_callback_type = std::function<void(const sf::Event&)>;

public:
    Chart(int x, int y, int w, int h, int num_of_points, float max_val, const std::string& font_name = "arial.ttf");

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    virtual void Handle(const sf::Event& event) override;

    void Enabled(bool enabled) override;
    bool Enabled() const override;

    void AddSignal(Signal* signal);
    void ChangeSignal(int idx, Signal* signal);

    // n_lines - number of one type of lines (vertical or horizontal), there are same number of other lines
    void                 CreateGrid(int n_lines);
    void                 CreateAxisMarkers();
    const sf::FloatRect& GraphRegion();
    float&               MaxVal();
    void                 EnableTriggerFrame();
    void                 DisableTriggerFrame();
    void                 ToggleDrawSignal(int idx);
    void                 ToggleDrawAllSignals();

    // Actions
    void OnKeyPress(const chart_callback_type& f);

private:
    static constexpr int m_margin{20};

    sf::RectangleShape m_background;
    sf::RectangleShape m_chart_region;
    sf::FloatRect      m_chart_rect;
    sf::VertexArray    m_outline;
    sf::VertexArray    m_axes;
    sf::VertexArray    m_grid;
    int                m_num_grid_lines{0};
    sf::Text           m_x_axis;
    sf::Text           m_y_axis;
    sf::Text           m_title;

    std::vector<sf::Text> m_x_axis_markers;
    std::vector<sf::Text> m_y_axis_markers;

    sf::Font m_font;

    std::vector<Signal*> m_signals;
    std::vector<bool>    m_draw_signal;
    float                m_max_val;
    int                  m_num_of_points;

    bool m_mouseover;

    chart_callback_type m_onKeyPress{nullptr};
};
