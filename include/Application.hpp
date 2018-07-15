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
		constexpr int max_val = 1000;
		chart = new gui::Chart(200, 10, 1640, 880, max_val);
		chart->CreateGrid(10);
		signals.reserve(N_CHANNELS);
		for (int i = 0; i < N_CHANNELS; ++i) {			
			signals.push_back(gui::Signal(10000, sf::Color(m_Colors[i]), chart->GetGraphRegion(), chart->GetMaxVal()));
			chart->AddSignal(&signals[signals.size() - 1]);
		}					

		// Connect button
		button_connect = new gui::Button(10, 50, "Connect", 120);
		button_connect->OnClick(button_connect_Click);

		// Comport textbox
		textbox_comport = new gui::Textbox(10, 10, "COM10");
		label_info = new gui::Label(10, 100, "");

		// Main window
		mainWindow = new Window(1850, 900, "Sorting Control");
		mainWindow->Attach(chart);
		mainWindow->Attach(button_connect);
		mainWindow->Attach(textbox_comport);
		mainWindow->Attach(label_info);
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
				communication->Write((uint8_t*)"USBY\n", 5);
				communication->Write((uint8_t*)"CSETF,1000\n", 11);
				communication->Write((uint8_t*)"VRBS,1\n", 7);
				communication->Write((uint8_t*)"CFILTERED\n", 10);
				//communication->Write((uint8_t*)"CRAW\n", 5);
				communication->Write((uint8_t*)"TRGFRM,1\n", 9);
				chart->DrawTriggerFrame();
				communication->Write((uint8_t*)"USBN\n", 5);

				thread_info = std::thread(Information);
				thread_get_data = std::thread(GetData);
			}			
		}
		else {
			communication->Disconnect();
			button_connect->SetText("Connect");

			thread_info.detach();
			thread_get_data.detach();
		}
	}	

	static void Information() {
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD coord{ 0, 0 };

		while (communication->IsConnected()) {
			//SetConsoleCursorPosition(handle, coord);
			//std::cout << "Availabile: " << std::to_string(communication->GetRxBufferLen()) << " bytes           \n";
			label_info->SetText(std::to_string(communication->GetRxBufferLen()) + " bytes");
			sf::sleep(sf::milliseconds(100));
		}		
	}
	 
	static void GetData() {
		static float fbuf[N_CHANNELS * DATA_PER_CHANNEL];
		static int cntr = 0;
		
		int tmp_cnt;

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
	static gui::Button *button_connect;
	static gui::Textbox *textbox_comport;
	static gui::Label	*label_info;
	static std::vector<gui::Signal> signals;

	static std::thread thread_info;
	static std::thread thread_get_data;
	
	static bool program_running;	
};
