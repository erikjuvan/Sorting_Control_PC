#pragma once

#include <mygui/Object.hpp>

class Window
{
private:
    using widget = mygui::Object;

    const sf::Color backgroundColor = sf::Color(235, 235, 235);

    sf::RenderWindow*    m_window;
    sf::Event*           m_event;
    std::vector<widget*> m_widgets;

    virtual void Events() final;
    virtual void Draw() final;

public:
    Window(int w, int h, const std::string& title, sf::Uint32 style = sf::Style::Default);
    ~Window();

    virtual void         Create(int w, int h, const std::string& title, sf::Uint32 style = sf::Style::Default) final;
    virtual void         Add(widget* w) final;
    virtual void         Update() final;
    virtual void         Show() final;
    virtual void         Hide() final;
    virtual bool         IsOpen() final;
    virtual sf::Vector2i GetPosition() const final;
    virtual void         SetPosition(const sf::Vector2i& position) final;
    virtual void         AlwaysOnTop(bool top) final;
    virtual void         MakeTransparent() final;
    virtual void         SetTransparency(sf::Uint8 alpha) final;
};
