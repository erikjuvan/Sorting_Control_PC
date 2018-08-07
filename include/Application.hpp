#pragma once

#include "Window.hpp"
#include "Gui.hpp"
#include "Communication.hpp"
#include "Helpers.hpp"

#include <thread>
#include <fstream>
#include <Windows.h>

class Application {
public:

	static void Init() {
		// MCU coms
		communication = new Communication();								

		///////////
		// Chart //
		///////////
		constexpr int N_SAMPLES = 10000;
		CreateChart(N_SAMPLES);

		/////////////
		// Buttons //
		/////////////
		button_connect = new gui::Button(10, 50, "Connect", 110);
		button_connect->OnClick(button_connect_Click);

		button_run = new gui::Button(10, 90, "Stopped", 110);
		button_run->OnClick(button_run_Click);

		button_trigger_frame = new gui::Button(10, 140, "Frame OFF", 110);
		button_trigger_frame->OnClick(button_trigger_frame_Click);

		button_capture = new gui::Button(140, 140, "Capture");
		button_capture->OnClick(button_capture_Click);

		button_toggle_usb_uart = new gui::Button(140, 50, "USB");
		button_toggle_usb_uart->OnClick(button_toggle_usb_uart_Click);

		button_view_mode = new gui::Button(140, 90, "Filtered");
		button_view_mode->OnClick(button_view_mode_Click);

		button_set_frequency = new gui::Button(10, 260, "Send");
		button_set_frequency->OnClick(button_set_frequency_Click);

		button_set_filter_params = new gui::Button(10, 380, "Send");
		button_set_filter_params->OnClick(button_set_filter_params_Click);
		
		button_set_times = new gui::Button(10, 500, "Send");
		button_set_times->OnClick(button_set_times_Click);

		button_record = new gui::Button(10, 600, "Record");
		button_record->OnClick(button_record_Click);

		//////////////
		// Texboxes //
		//////////////
		textbox_comport = new gui::Textbox(10, 10, "COM", 80);
		textbox_frequency = new gui::Textbox(10, 220, "10000", 80);
		textbox_filter_params = new gui::Textbox(10, 340, "0.01,0.03,0.03,7.0", 170);
		textbox_times = new gui::Textbox(10, 460, "0,100,1000", 120);

		////////////
		// Labels //
		////////////			
		label_frequency = new gui::Label(10, 190, "Sample frequency:");
		label_filter_params = new gui::Label(10, 310, "Filter params(a1,a2,a3,thr):");
		label_times = new gui::Label(10, 430, "Times (dly, dur, blind):");
		label_info_rx_bytes = new gui::Label(10, 700, "Rx buf: 0 bytes");

		////////////////
		// Checkboxes //
		////////////////
		checkbox_only_show_framed = new gui::Checkbox(10, 550, "Only show framed");
		checkbox_only_show_framed->OnClick(checkbox_only_show_framed_Clicked);

		/////////////////
		// Main window //
		/////////////////
		mainWindow = new Window(1850, 900, "Sorting Control");
		mainWindow->Attach(chart);		
		// Buttons
		mainWindow->Attach(button_connect);
		mainWindow->Attach(button_run);
		mainWindow->Attach(button_trigger_frame);
		mainWindow->Attach(button_set_frequency);
		mainWindow->Attach(button_set_filter_params);
		mainWindow->Attach(button_set_times);
		mainWindow->Attach(button_toggle_usb_uart);
		mainWindow->Attach(button_view_mode);
		mainWindow->Attach(button_capture);
		mainWindow->Attach(button_record);
		// Texboxes
		mainWindow->Attach(textbox_comport);
		mainWindow->Attach(textbox_frequency);
		mainWindow->Attach(textbox_filter_params);
		mainWindow->Attach(textbox_times);
		// Labels
		mainWindow->Attach(label_info_rx_bytes);
		mainWindow->Attach(label_frequency);
		mainWindow->Attach(label_filter_params);
		mainWindow->Attach(label_times);
		// Checkboxes
		mainWindow->Attach(checkbox_only_show_framed);

		// Initial parameters from file init
		InitFromFile("config.txt");

		m_running = Running::STOPPED;
		m_mode = Mode::LIVE;
		m_view = View::FILTERED;
		m_capture = Capture::OFF;
	}

	static void Run() {	
		mainWindow->Run();
	}

private:

	static void CreateChart(int n_samples) {
		m_n_samples = n_samples;
		chart = new gui::Chart(240, 10, 1600, 880, m_n_samples, 100);
		chart->CreateGrid(9);
		chart->OnKeyPress(chart_OnKeyPress);
		signals.clear();
		signals.reserve(N_CHANNELS);
		for (int i = 0; i < N_CHANNELS; ++i) {
			signals.push_back(gui::Signal(m_n_samples, sf::Color(m_Colors[i]), chart->GetGraphRegion(), chart->GetMaxVal()));
			chart->AddSignal(&signals[signals.size() - 1]);
		}
	}

	static void InitFromFile(const std::string& file_name) {
		std::ifstream in_file(file_name);		
		std::string str;
		std::vector<std::string> tokens;
		if (in_file.is_open()) {
			while (std::getline(in_file, str)) {
				tokens.push_back(str);
			}
			in_file.close();
		}

		for (int i = 0; i < tokens.size(); ++i) {
			switch (i) {
			case 0: // COM port
				textbox_comport->SetText(tokens[i]);
				break;
			}
		}
	}

	static void button_connect_Click() {
		if (!communication->IsConnected()) {
			if (communication->Connect(textbox_comport->GetText())) {

				button_connect->SetText("Disconnect");
				communication->Write("USBY\n");
				button_set_frequency_Click();
				button_set_filter_params_Click();
				button_set_times_Click();

				communication->Write("CFILTERED\n");
				communication->Write("VRBS,1\n");

				thread_info = std::thread(Information);
				thread_get_data = std::thread(GetData);
			}			
		}
		else {			
			communication->Write("USBY\n");			
			communication->Disconnect();			
			button_connect->SetText("Connect");

			thread_info.detach();
			thread_get_data.detach();
		}
	}
	
	static void button_run_Click() {		
		if (m_running == Running::STOPPED) {
			m_running = Running::RUNNING;
			button_run->SetText("Running");
		}
		else if (m_running == Running::RUNNING) {
			m_running = Running::STOPPED;
			button_run->SetText("Stopped");
		}
	}

	static void button_toggle_usb_uart_Click() {
		static enum class Com { USB, UART } com{ Com::USB };

		if (com == Com::USB) {
			com = Com::UART;
			communication->Write("USBN\n");
			button_toggle_usb_uart->SetText("UART");			
		}
		else if (com == Com::UART) {
			com = Com::USB;
			communication->Write("USBY\n");
			button_toggle_usb_uart->SetText("USB");			
		}
	}

	static void button_trigger_frame_Click() {
		static bool trigger_frame = false;
		if (trigger_frame == false) {
			trigger_frame = true;
			communication->Write("TRGFRM,1\n");
			chart->EnableTriggerFrame();			
			button_trigger_frame->SetText("Frame ON");
		}
		else {
			trigger_frame = false;
			communication->Write("TRGFRM,0\n");
			chart->DisableTriggerFrame();			
			button_trigger_frame->SetText("Frame OFF");
		}
	}	

	static void button_set_frequency_Click() {
		communication->Write("CSETF," + textbox_frequency->GetText() + "\n");
	}

	static void button_set_filter_params_Click() {		
		communication->Write("CPARAMS," + textbox_filter_params->GetText() + "\n");
		// Extract threashold from the string
		std::string str = textbox_filter_params->GetText();
		auto idx = str.find_last_of(',') + 1;
		str = str.substr(idx);

		chart->SetTriggerFrame(std::stof(str));
	}

	static void button_set_times_Click() {
		communication->Write("CTIMES," + textbox_times->GetText() + "\n");
	}

	static void button_view_mode_Click() {
		if (m_view == View::FILTERED) {
			m_view = View::RAW;
			communication->Write("CRAW\n");
			button_view_mode->SetText("Raw");
		}
		else if (m_view == View::RAW) {
			m_view = View::FILTERED;
			communication->Write("CFILTERED\n");
			button_view_mode->SetText("Filtered");			
		}
	}

	static void button_capture_Click() {		
		if (m_capture == Capture::OFF) {
			m_capture = Capture::ON;
			button_capture->SetColor(sf::Color::Green);
		}
		else if (m_capture == Capture::ON) {
			m_capture = Capture::OFF;
			button_capture->ResetColor();
		}
	}

	static void button_record_Click() {
		if (m_mode == Mode::LIVE) {
			m_mode = Mode::RECORD;
			button_record->SetColor(sf::Color::Red);
		} 
		else if (m_mode == Mode::RECORD) {
			m_mode = Mode::LIVE;
			button_record->ResetColor();
		}
	}

	static void checkbox_only_show_framed_Clicked() {
		for (auto& s : signals) {
			s.OnlyDrawOnTrigger(checkbox_only_show_framed->IsChecked());
		}
	}

	static void chart_OnKeyPress(const sf::Event& event) {
		if (m_mode == Mode::RECORD && m_running == Running::STOPPED) {
			if (event.key.code == sf::Keyboard::Left) {

			}
			if (event.key.code == sf::Keyboard::Right) {

			}
		}
	}

	static void Information() {
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD coord{ 0, 0 };
		static int cnt = 0;

		while (communication->IsConnected()) {
			//SetConsoleCursorPosition(handle, coord);
			//std::cout << "Availabile: " << std::to_string(communication->GetRxBufferLen()) << " bytes           \n";
			label_info_rx_bytes->SetText(std::to_string(cnt++) + " Rx buf: " + std::to_string(communication->GetRxBufferLen()) + " bytes");
			sf::sleep(sf::milliseconds(100));
		}		
	}
	 
	static void GetData() {
		static float fbuf[N_CHANNELS * DATA_PER_CHANNEL];
		static int cntr = 0;
		static bool thr_missed;

		while (communication->IsConnected()) {

			if (m_running == Running::RUNNING) {

				while (communication->IsConnected()) {
					communication->Read(fbuf, 4);
					if (*((uint32_t*)&fbuf[0]) == 0xDEADBEEF) {
						break;
					}
				}

				int read = communication->Read(fbuf, sizeof(fbuf));
				if (read > 0) {
					thr_missed = false;
					float* fbuf_tmp = fbuf;
					for (int ch = 0; ch < N_CHANNELS; ++ch) {
						signals[ch].Edit(fbuf_tmp, cntr * DATA_PER_CHANNEL, DATA_PER_CHANNEL);
						if (signals[ch].ThreasholdMissed() && m_capture == Capture::ON && !thr_missed) {
							button_run_Click();	// only change state here
							thr_missed = true;
							// we don't break out, because we want to draw out the remaining buffer
						}

						fbuf_tmp += DATA_PER_CHANNEL;
					}
					if (++cntr >= (m_n_samples / DATA_PER_CHANNEL)) {
						cntr = 0;
					}
				}

			}
		}		
	}

private:	
	static constexpr int DATA_PER_CHANNEL = 100;
	static constexpr int N_CHANNELS { 8 };
	static constexpr uint32_t m_Colors[10]{ 0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0xFFFF00FF, 0x00FFFFFF, 0xFF00FFFF, 0xFF8000FF, 0xC0C0C0FF, 0x800000FF, 0x808000FF };		

	enum class Running { STOPPED, RUNNING };
	enum class Mode { LIVE, RECORD };
	enum class View { RAW, FILTERED };
	enum class Capture { ON, OFF };

	static int m_n_samples;

	static Running m_running;
	static Mode	m_mode;
	static View m_view;	
	static Capture m_capture;

	static Communication *communication;
	static Window *mainWindow;

	static gui::Chart *chart;
	// Button
	static gui::Button *button_connect;
	static gui::Button *button_run;
	static gui::Button *button_trigger_frame;
	static gui::Button *button_set_frequency;
	static gui::Button *button_set_filter_params;
	static gui::Button *button_set_times;
	static gui::Button *button_toggle_usb_uart;
	static gui::Button *button_view_mode;
	static gui::Button *button_capture;
	static gui::Button *button_record;
	// Texbox
	static gui::Textbox *textbox_comport;
	static gui::Textbox *textbox_frequency;
	static gui::Textbox *textbox_filter_params;
	static gui::Textbox *textbox_times;

	// Labels	
	static gui::Label	*label_frequency;
	static gui::Label	*label_filter_params;
	static gui::Label	*label_times;
	static gui::Label	*label_info_rx_bytes;

	// Checkboxes
	static gui::Checkbox *checkbox_only_show_framed;

	static std::vector<gui::Signal> signals;

	static std::thread thread_info;
	static std::thread thread_get_data;
};
