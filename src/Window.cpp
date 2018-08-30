#include "Window.hpp"

Window::Window(int w, int h, const char* title) {
	m_render_window = new sf::RenderWindow(sf::VideoMode(w, h), title, sf::Style::Close | sf::Style::Titlebar);
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
	m_render_window->clear(backgroundColor);

	for (const auto& o : m_objects) {
		m_render_window->draw(*o);
	}
	
	m_render_window->display();
}

void Window::EventHandler() {
	while (m_render_window->pollEvent(*m_event)) {
		// "close requested" event: we close the window
		if (m_event->type == sf::Event::Closed) {
			m_render_window->close();
		}
		for (auto& m : m_objects) {
			m->Handle(*m_event);
		}
	}
}

void Window::Run() {

	if (m_render_window->isOpen()) {
		EventHandler();
		Draw();
	}

}

bool Window::IsOpen() {
	return m_render_window->isOpen();
}

void Window::Show() {
	m_render_window->setVisible(true);
}

void Window::Hide() {
	m_render_window->setVisible(false);
}