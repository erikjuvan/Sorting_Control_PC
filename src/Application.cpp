#include "Application.hpp"

Communication *Application::communication;
Window *Application::mainWindow;

gui::Chart *Application::chart;
gui::Button *Application::button_connect;
gui::Textbox *Application::textbox_comport;
gui::Label	*Application::label_info;

std::thread Application::thread_info;
std::thread Application::thread_get_data;

bool Application::program_running;


