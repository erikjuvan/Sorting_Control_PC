#include "Window.hpp"

Window::Window(int w, int h, const std::string& title, sf::Uint32 style)
{
    m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode(w, h), title, style);
    m_event  = std::make_unique<sf::Event>();

    //m_window->setFramerateLimit(60); // currently already m_running at 60 fps even without limit
}

void Window::Events()
{
    while (m_window->pollEvent(*m_event)) {
        if (m_event->type == sf::Event::Closed) {
            m_window->close();
        }

        for (auto& w : m_widgets) {
            w->Handle(*m_event);
        }
    }
}

void Window::Create(int w, int h, const std::string& title, sf::Uint32 style)
{
    m_window->create(sf::VideoMode(w, h), title, style);
}

void Window::Close()
{
    m_window->close();
}

void Window::Add(std::shared_ptr<Widget> const& widget)
{
    m_widgets.push_back(widget);
}

void Window::Draw()
{
    m_window->clear(backgroundColor);

    for (const auto& w : m_widgets) {
        m_window->draw(*w);
    }

    m_window->display();
}

void Window::Update()
{
    Events();
    Draw();
}

void Window::SetVisible(bool visible)
{
    m_window->setVisible(visible);
}

bool Window::IsOpen() const
{
    return m_window->isOpen();
}

sf::Vector2i Window::GetPosition() const
{
    return m_window->getPosition();
}

void Window::SetPosition(const sf::Vector2i& position)
{
    m_window->setPosition(position);
}

#if defined(_WIN32)

#include <Windows.h>

void Window::AlwaysOnTop(bool top)
{
    SetWindowPos(m_window->getSystemHandle(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Window::MakeTransparent()
{
    HWND hwnd = m_window->getSystemHandle();
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
}

void Window::SetTransparency(sf::Uint8 alpha)
{
    SetLayeredWindowAttributes(m_window->getSystemHandle(), 0, alpha, LWA_ALPHA);
}

#else

void Window::AlwaysOnTop(bool top)
{
}

void Window::MakeTransparent()
{
}

void Window::SetTransparency(sf::Uint8 alpha)
{
}

#endif