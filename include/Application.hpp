#pragma once

#include "Window.hpp"
#include "Gui.hpp"
#include "Communication.hpp"
#include "Helpers.hpp"

#include <thread>
#include <Windows.h>

class Application {
public:

	static void Init() {
		// MCU coms
		communication = new Communication();				

		// Chart
		static constexpr int N = 10000;
		chart = new gui::Chart(240, 10, 1600, 880, N, 1000);
		chart->CreateGrid(9);
		signals.reserve(N_CHANNELS);
		for (int i = 0; i < N_CHANNELS; ++i) {			
			signals.push_back(gui::Signal(N, sf::Color(m_Colors[i]), chart->GetGraphRegion(), chart->GetMaxVal()));
			chart->AddSignal(&signals[signals.size() - 1]);
		}					

		/////////////
		// Buttons //
		/////////////
		button_connect = new gui::Button(10, 50, "Connect", 110);
		button_connect->OnClick(button_connect_Click);

		button_trigger_frame = new gui::Button(10, 120, "Frame OFF", 120);
		button_trigger_frame->OnClick(button_trigger_frame_Click);

		button_set_frequency = new gui::Button(10, 260, "Send");
		button_set_frequency->OnClick(button_set_frequency_Click);

		button_set_filter_params = new gui::Button(10, 380, "Send");
		button_set_filter_params->OnClick(button_set_filter_params_Click);
		
		button_set_times = new gui::Button(10, 500, "Send");
		button_set_times->OnClick(button_set_times_Click);

		button_toggle_config_run = new gui::Button(140, 10, "Config");
		button_toggle_config_run->OnClick(button_toggle_config_run_Click);

		button_view_mode = new gui::Button(140, 50, "Filtered");
		button_view_mode->OnClick(button_view_mode_Click);

		//////////////
		// Texboxes //
		//////////////
		textbox_comport = new gui::Textbox(10, 10, "COM10", 80);
		textbox_frequency = new gui::Textbox(10, 220, "10000", 80);
		textbox_filter_params = new gui::Textbox(10, 340, "0.01,0.03,0.03,7.0", 170);
		textbox_times = new gui::Textbox(10, 460, "0,100,1000", 120);

		////////////
		// Labels //
		////////////			
		label_frequency = new gui::Label(10, 190, "Sample frequency:");
		label_filter_params = new gui::Label(10, 310, "Filter params(a1,a2,a3,thr):");
		label_times = new gui::Label(10, 430, "Times (dly, dur, blind):");
		label_info = new gui::Label(10, 700, "Rx buf: 0 bytes");

		/////////////////
		// Main window //
		/////////////////

		mainWindow = new Window(1850, 900, "Sorting Control");
		mainWindow->Attach(chart);
		chart->SetParentWindow(mainWindow->GetWindow());
		// Buttons
		mainWindow->Attach(button_connect);
		mainWindow->Attach(button_trigger_frame);
		mainWindow->Attach(button_set_frequency);
		mainWindow->Attach(button_set_filter_params);
		mainWindow->Attach(button_set_times);
		mainWindow->Attach(button_toggle_config_run);
		mainWindow->Attach(button_view_mode);
		// Texboxes
		mainWindow->Attach(textbox_comport);
		mainWindow->Attach(textbox_frequency);
		mainWindow->Attach(textbox_filter_params);
		mainWindow->Attach(textbox_times);
		// Labels
		mainWindow->Attach(label_info);
		mainWindow->Attach(label_frequency);
		mainWindow->Attach(label_filter_params);
		mainWindow->Attach(label_times);
	}

	static void Run() {	
		mainWindow->Run();

		delete mainWindow;
		delete chart;
		delete button_connect;
		delete textbox_comport;
		delete communication;
	}

private:

	static void button_connect_Click() {
		if (!communication->IsConnected()) {
			if (communication->Connect(textbox_comport->GetText())) {
				button_connect->SetText("Disconnect");
				communication->Write("USBY\n");
				//communication->Write("CSETF,10000\n");
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
	
	static void button_toggle_config_run_Click() {
		static enum class State { CONFIG, RUN } state{ State::CONFIG };

		if (state == State::CONFIG) {
			communication->Write("USBN\n");
			button_toggle_config_run->SetText("Running");
			state = State::RUN;
		}
		else if (state == State::RUN) {
			communication->Write("USBY\n");
			button_toggle_config_run->SetText("Config");
			state = State::CONFIG;
		}
	}

	static void button_trigger_frame_Click() {
		static bool trigger_frame = false;
		if (trigger_frame == false) {
			communication->Write((uint8_t*)"TRGFRM,1\n", 9);
			chart->EnableTriggerFrame();
			trigger_frame = true;
			button_trigger_frame->SetText("Frame ON");
		}
		else {
			communication->Write((uint8_t*)"TRGFRM,0\n", 9);
			chart->DisableTriggerFrame();
			trigger_frame = false;
			button_trigger_frame->SetText("Frame OFF");
		}
	}	

	static void button_set_frequency_Click() {
		communication->Write("CSETF," + textbox_frequency->GetText() + "\n");
	}

	static void button_set_filter_params_Click() {
		communication->Write("CPARAMS," + textbox_filter_params->GetText() + "\n");
	}

	static void button_set_times_Click() {
		communication->Write("CTIMES," + textbox_times->GetText() + "\n");
	}

	static void button_view_mode_Click() {
		static enum class View { RAW, FILTERED } view{ View::FILTERED };

		if (view == View::FILTERED) {
			communication->Write("CRAW\n");
			button_view_mode->SetText("Raw");
			view = View::RAW;
		}
		else if (view == View::RAW) {
			communication->Write("CFILTERED\n");
			button_view_mode->SetText("Filtered");
			view = View::FILTERED;
		}
	}

	static void Information() {
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD coord{ 0, 0 };

		while (communication->IsConnected()) {
			//SetConsoleCursorPosition(handle, coord);
			//std::cout << "Availabile: " << std::to_string(communication->GetRxBufferLen()) << " bytes           \n";
			label_info->SetText("Rx buf: " + std::to_string(communication->GetRxBufferLen()) + " bytes");
			sf::sleep(sf::milliseconds(100));
		}		
	}
	 
	static void GetData() {
		static float fbuf[N_CHANNELS * DATA_PER_CHANNEL];
		static int cntr = 0;

		while (communication->IsConnected()) {

			while (communication->IsConnected()) {
				communication->Read(fbuf, 4);
				if (*((uint32_t*)&fbuf[0]) == 0xDEADBEEF) {
					break;
				}				
			}			

			if (int read = communication->Read(fbuf, sizeof(fbuf))) {
				float* fbuf_tmp = fbuf;
				for (int ch = 0; ch < N_CHANNELS; ++ch) {
					signals[ch].Edit(fbuf_tmp, cntr * DATA_PER_CHANNEL, DATA_PER_CHANNEL);
					fbuf_tmp += DATA_PER_CHANNEL;
				}
				if (++cntr >= 100) {
					cntr = 0;
				}
			}
		}		
	}




private:
	static constexpr int DATA_PER_CHANNEL = 100;
	static constexpr int N_CHANNELS { 8 };
	static constexpr uint32_t m_Colors[10]{ 0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0xFFFF00FF, 0x00FFFFFF, 0xFF00FFFF, 0xFF8000FF, 0xC0C0C0FF, 0x800000FF, 0x808000FF };

	static Communication *communication;
	static Window *mainWindow;

	static gui::Chart *chart;
	// Button
	static gui::Button *button_connect;
	static gui::Button *button_trigger_frame;
	static gui::Button *button_set_frequency;
	static gui::Button *button_set_filter_params;
	static gui::Button *button_set_times;
	static gui::Button *button_toggle_config_run;
	static gui::Button *button_view_mode;

	// Texbox
	static gui::Textbox *textbox_comport;
	static gui::Textbox *textbox_frequency;
	static gui::Textbox *textbox_filter_params;
	static gui::Textbox *textbox_times;

	// Labels
	static gui::Label	*label_info;
	static gui::Label	*label_frequency;
	static gui::Label	*label_filter_params;
	static gui::Label	*label_times;

	static std::vector<gui::Signal> signals;

	static std::thread thread_info;
	static std::thread thread_get_data;
	
	static bool program_running;	
};
