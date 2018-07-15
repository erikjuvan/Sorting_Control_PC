#include "Window.hpp"

Window::Window(int w, int h, const char* title) {
	m_window = new sf::RenderWindow(sf::VideoMode(w, h), title, sf::Style::Close | sf::Style::Titlebar);
	//m_window->setFramerateLimit(60);

	m_event = new sf::Event();
}

Window::~Window() {
}

void Window::Attach(gui::Object* d) {
	m_objects.push_back(d);
}

void Window::Draw() {
	static const auto backgroundColor = sf::Color(235, 235, 235);
	m_window->clear(backgroundColor);

	for (const auto& o : m_objects) {
		m_window->draw(*o);
	}
	
	m_window->display();
}

void Window::EventHandler() {
	while (m_window->pollEvent(*m_event))
	{
		// "close requested" event: we close the window
		if (m_event->type == sf::Event::Closed) {
			m_window->close();
		}
		for (auto& m : m_objects) {
			m->Handle(*m_event);
		}
	}
}

void Window::Run() {

	while (m_window->isOpen()) {
		EventHandler();
		Draw();
	}

}

bool Window::IsOpen() {
	return m_window->isOpen();
}