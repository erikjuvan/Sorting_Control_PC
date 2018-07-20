#include "Application.hpp"

Communication *Application::communication;
Window *Application::mainWindow;

Application::Running Application::m_running;
Application::View Application::m_view;
Application::Capture Application::m_capture;

gui::Chart *Application::chart;

// Button
gui::Button *Application::button_connect;
gui::Button *Application::button_run;
gui::Button *Application::button_trigger_frame;
gui::Button *Application::button_set_frequency;
gui::Button *Application::button_set_filter_params;
gui::Button *Application::button_set_times;
gui::Button *Application::button_toggle_usb_uart;
gui::Button *Application::button_view_mode;
gui::Button *Application::button_capture;

// Texbox
gui::Textbox *Application::textbox_comport;
gui::Textbox *Application::textbox_frequency;
gui::Textbox *Application::textbox_filter_params;
gui::Textbox *Application::textbox_times;

gui::Label	*Application::label_frequency;
gui::Label	*Application::label_filter_params;
gui::Label	*Application::label_times;
gui::Label	*Application::label_info_rx_bytes;

std::vector<gui::Signal> Application::signals;

std::thread Application::thread_info;
std::thread Application::thread_get_data;

bool Application::program_running;


