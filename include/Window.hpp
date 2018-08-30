#pragma once

#include <mygui/Object.hpp>

class Window {
public:
	Window(int w, int h, const char* title);
	~Window();

	void Draw();
	void EventHandler();
	void Run();
	void Attach(mygui::Object* d);
	bool IsOpen();	
	void Show();
	void Hide();

private:	
	sf::RenderWindow *m_render_window;
	sf::Event *m_event;
	std::vector<mygui::Object*> m_objects;
};

