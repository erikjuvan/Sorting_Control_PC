#pragma once

#include "Window.hpp"
#include "Application.hpp"
#include <mygui/Button.hpp>
#include <mygui/Label.hpp>
#include <mygui/Chart.hpp>
#include <mygui/Textbox.hpp>
#include <mygui/Checkbox.hpp>

class MainWindow {
private:
	static constexpr uint32_t m_Colors[10]{ 0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0xFFFF00FF, 0x00FFFFFF, 0xFF00FFFF, 0xFF8000FF, 0xC0C0C0FF, 0x800000FF, 0x808000FF };

	Window	*m_window;
	Application *m_application;
	
public:
	// Methods
	//////////

	MainWindow(int w, int h, const char* title, Application* application);
	~MainWindow();

	inline void Run() { m_window->Run(); }
	bool IsOpen();
	void RunClick();
	void CreateChart(int n_sample);

	static void button_connect_Click(void*);
	static void button_run_Click(void*);
	static void button_toggle_usb_uart_Click(void*);
	static void button_trigger_frame_Click(void*);
	static void button_set_frequency_Click(void*);
	static void button_set_filter_params_Click(void*);
	static void button_set_times_Click(void*);
	static void button_view_mode_Click(void*);
	static void button_capture_Click(void*);
	static void button_record_Click(void*);
	static void button_analysis_window_Click(void*);
	static void checkbox_only_show_framed_Clicked(void*);
	static void label_info_detected_in_window_Clicked(void*);
	static void label_info_detected_out_window_Clicked(void*);
	static void label_info_signal_missed_Clicked(void*);
	static void chart_OnKeyPress(void*, const sf::Event&);


	// Members
	//////////

	mygui::Chart *chart;
	// Button
	mygui::Button *button_connect;
	mygui::Button *button_run;
	mygui::Button *button_trigger_frame;
	mygui::Button *button_set_frequency;
	mygui::Button *button_set_filter_params;
	mygui::Button *button_set_times;
	mygui::Button *button_toggle_usb_uart;
	mygui::Button *button_view_mode;
	mygui::Button *button_capture;
	mygui::Button *button_record;
	mygui::Button *button_analysis_window;
	// Texbox
	mygui::Textbox *textbox_comport;
	mygui::Textbox *textbox_frequency;
	mygui::Textbox *textbox_filter_params;
	mygui::Textbox *textbox_times;

	// Labels	
	mygui::Label	*label_frequency;
	mygui::Label	*label_filter_params;
	mygui::Label	*label_times;
	mygui::Label	*label_info_rx_bytes;
	mygui::Label	*label_info_detected_in_window;
	mygui::Label	*label_info_detected_out_window;
	mygui::Label	*label_info_signal_missed;

	// Checkboxes
	mygui::Checkbox *checkbox_only_show_framed;

	std::vector<mygui::Signal> signals;
	std::vector<mygui::Signal> recorded_signals;
};


