#pragma once

#include "Signal.hpp"
#include <mygui/Object.hpp>

class Chart : public mygui::Object
{
private:
    using chart_callback_type = std::function<void(const sf::Event&)>;

    const int m_margin{20};

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

    std::vector<std::shared_ptr<Signal>> m_signals;
    std::vector<bool>                    m_draw_signal;
    bool                                 m_draw_all_signals = true;

    std::shared_ptr<float> m_max_val;

    int m_num_of_points;

    bool m_mouseover;

    chart_callback_type m_onKeyPress{nullptr};

public:
    Chart(ResManager& rm, int x, int y, int w, int h, int num_of_points, float max_val);

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    virtual void Handle(const sf::Event& event) override;

    virtual void Enabled(bool enabled) override;
    virtual bool Enabled() const override;

    void AddSignal(std::shared_ptr<Signal> const& signal);
    void ChangeSignal(int idx, std::shared_ptr<Signal> const& signal);

    // n_lines - number of one type of lines (vertical or horizontal), there are same number of other lines
    void                         CreateGrid(int n_lines);
    void                         CreateAxisMarkers();
    const sf::FloatRect&         GraphRegion();
    std::shared_ptr<float const> MaxVal();
    void                         EnableTriggerFrame();
    void                         DisableTriggerFrame();
    void                         ToggleDrawSignal(int idx);
    void                         ToggleDrawAllSignals();

    // Actions
    void OnKeyPress(const chart_callback_type& f);
};
