#include "Window.hpp"

Window::Window(int w, int h, const std::string& title, sf::Uint32 style) {
	m_window = new sf::RenderWindow(sf::VideoMode(w, h), title, style);
	m_event = new sf::Event();
}

Window::~Window() {
	delete m_window;
	delete m_event;
}

void Window::Events() {
	while (m_window->pollEvent(*m_event)) {
		if (m_event->type == sf::Event::Closed) {
			m_window->close();
		}

		for (const auto& w : m_widgets) {
			w->Handle(*m_event);
		}
	}
}

void Window::Create(int w, int h, const std::string& title, sf::Uint32 style) {
	m_window->create(sf::VideoMode(w, h), title, style);
}

void Window::Add(widget* w) {
	m_widgets.push_back(w);
}

void Window::Draw() {
	m_window->clear(backgroundColor);

	for (const auto& w : m_widgets) {
		m_window->draw(*w);
	}

	m_window->display();
}

void Window::Update() {
	Events();
	Draw();
}

void Window::Show() {
	m_window->setVisible(true);
}

void Window::Hide() {
	m_window->setVisible(false);
}

bool Window::IsOpen() {
	return m_window->isOpen();
}

sf::Vector2i Window::GetPosition() const {
	return m_window->getPosition();
}

void Window::SetPosition(const sf::Vector2i& position) {
	m_window->setPosition(position);
}

#include <Windows.h>

void Window::AlwaysOnTop(bool top) {
	SetWindowPos(m_window->getSystemHandle(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Window::MakeTransparent() {
	HWND hwnd = m_window->getSystemHandle();
	SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
}

void Window::SetTransparency(sf::Uint8 alpha) {	
	SetLayeredWindowAttributes(m_window->getSystemHandle(), 0, alpha, LWA_ALPHA);
}