#pragma once

#include "Window.hpp"
#include "Application.hpp"

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

	gui::Chart *chart;
	// Button
	gui::Button *button_connect;
	gui::Button *button_run;
	gui::Button *button_trigger_frame;
	gui::Button *button_set_frequency;
	gui::Button *button_set_filter_params;
	gui::Button *button_set_times;
	gui::Button *button_toggle_usb_uart;
	gui::Button *button_view_mode;
	gui::Button *button_capture;
	gui::Button *button_record;
	gui::Button *button_analysis_window;
	// Texbox
	gui::Textbox *textbox_comport;
	gui::Textbox *textbox_frequency;
	gui::Textbox *textbox_filter_params;
	gui::Textbox *textbox_times;

	// Labels	
	gui::Label	*label_frequency;
	gui::Label	*label_filter_params;
	gui::Label	*label_times;
	gui::Label	*label_info_rx_bytes;
	gui::Label	*label_info_detected_in_window;
	gui::Label	*label_info_detected_out_window;
	gui::Label	*label_info_signal_missed;

	// Checkboxes
	gui::Checkbox *checkbox_only_show_framed;

	std::vector<gui::Signal> signals;
	std::vector<gui::Signal> recorded_signals;
};


