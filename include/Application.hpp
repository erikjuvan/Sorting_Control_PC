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
		chart = new gui::Chart(200, 10, 1400, 700, 1000);
		chart->CreateGrid(4);
		for (int i = 0; i < N_CHANNELS; ++i)
			chart->AddCurve(20000, sf::Color(m_Colors[i]));

		// Connect button
		button_connect = new gui::Button(10, 50, "Connect", 120);
		button_connect->OnClick(button_connect_Click);

		// Comport textbox
		textbox_comport = new gui::Textbox(10, 10, "COM10");
		label_info = new gui::Label(10, 100, "");

		// Main window
		mainWindow = new Window(1610, 720, "Sorting Control");
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
				communication->Write((uint8_t*)"VRBS,1\n", 7);

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
		static float buf[N_CHANNELS * DATA_PER_CHANNEL];
		static int cntr = 0;
		std::vector<uint32_t> ints;

		while (communication->IsConnected()) {

			while (communication->IsConnected()) {
				communication->Read(buf, 4);
				if (*((uint32_t*)&buf[0]) == 0xDEADBEEF) {				
					break;
				}				
			}				

			if (int read = communication->Read(buf, sizeof(buf))) {
				for (int ch = 0; ch < N_CHANNELS; ++ch) {
					std::vector<float> vec;
					vec.insert(vec.begin(), &buf[ch*DATA_PER_CHANNEL], &buf[ch*DATA_PER_CHANNEL + DATA_PER_CHANNEL]);
					chart->CurveEditPoints(ch, cntr * 100, 100, vec);
				}
				if (++cntr >= 200) {
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

	static std::thread thread_info;
	static std::thread thread_get_data;
	
	static bool program_running;	
};
