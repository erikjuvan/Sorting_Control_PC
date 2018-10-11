#pragma once

#include <mygui/Object.hpp>

class Window
{
private:
    sf::Color backgroundColor = sf::Color(235, 235, 235);

protected:
    using widget = mygui::Object;

    sf::RenderWindow*    m_window;
    sf::Event*           m_event;
    std::vector<widget*> m_widgets;

    virtual void Events();
    virtual void Draw();

public:
    Window(int w, int h, const std::string& title, sf::Uint32 style = sf::Style::Default);
    ~Window();

    void         Create(int w, int h, const std::string& title, sf::Uint32 style = sf::Style::Default);
    void         Add(widget* w);
    void         Update();
    void         Show();
    void         Hide();
    bool         IsOpen();
    sf::Vector2i GetPosition() const;
    void         SetPosition(const sf::Vector2i& position);

    void AlwaysOnTop(bool top);
    void MakeTransparent();
    void SetTransparency(sf::Uint8 alpha);
};
