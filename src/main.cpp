#include <iostream>

#include "Helpers.hpp"
#include "Window.hpp"
#include "Gui.hpp"


int main(int argc, char *argv[]) {

	Window *mainWindow = new Window(1000, 600, "Sorting Control");
	gui::Chart *chart = new gui::Chart(20, 20, 800, 500);
	gui::Button *btn = new gui::Button("hello", 10, 10);

	mainWindow->Attach(chart);
	mainWindow->Attach(btn);

	mainWindow->Run();

	return 0;
}