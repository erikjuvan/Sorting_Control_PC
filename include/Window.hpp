#pragma once

#include "Gui.hpp"

class Window {
public:
	Window(int w, int h, const char* title);
	~Window();

	void Draw();
	void EventHandler();
	void Run();
	void Attach(gui::Object* d);
	bool IsOpen();
	sf::RenderWindow* GetWindow() { return m_window; }

private:	
	sf::RenderWindow *m_window;
	sf::Event *m_event;
	std::vector<gui::Object*> m_objects;
};

