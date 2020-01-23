#pragma once

#include <memory>
#include <mygui/Object.hpp>

class Window
{
protected:
    using Widget = mygui::Object;

    const sf::Color backgroundColor = sf::Color(235, 235, 235);

    std::unique_ptr<sf::RenderWindow>    m_window;
    std::unique_ptr<sf::Event>           m_event;
    std::vector<std::shared_ptr<Widget>> m_widgets;

    virtual void Events();
    virtual void Draw();

public:
    Window(int w, int h, const std::string& title, sf::Uint32 style = sf::Style::Default);

    void         Create(int w, int h, const std::string& title, sf::Uint32 style = sf::Style::Default);
    void         Close();
    void         Add(std::shared_ptr<Widget> const& widget);
    void         Update();
    void         SetVisible(bool visible);
    bool         IsOpen() const;
    sf::Vector2i GetPosition() const;
    void         SetPosition(const sf::Vector2i& position);
    void         AlwaysOnTop(bool top);
    void         MakeTransparent();
    void         SetTransparency(sf::Uint8 alpha);
    void         SetTitle(std::string const& title) { m_window->setTitle(title); }
};
