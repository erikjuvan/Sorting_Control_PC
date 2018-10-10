#include "Application.hpp"
#include "MainWindow.hpp"
#include "AnalysisWindow.hpp"
#include "Communication.hpp"
#include <fstream>
#include <thread>

Communication	*g_communication;
MainWindow		*g_mainWindow;
AnalysisWindow	*g_analysisWindow;

Running g_running;
Record	g_record;
View	g_view;
TriggerFrame g_triggerframe;

static std::thread g_thread_info;
static std::thread g_thread_get_data;

int g_n_samples;

static void Information() {
	static int cnt = 0;
	static int detected_in_window_cnt = 0, detected_out_window_cnt = 0, signal_missed_cnt = 0;

	while (g_mainWindow->IsOpen()) {
		if (g_communication->IsConnected()) {
			g_mainWindow->label_info_rx_bytes->SetText(std::to_string(cnt++) + " Rx buf: " + std::to_string(g_communication->GetRxBufferLen()) + " bytes");

			detected_in_window_cnt = detected_out_window_cnt = signal_missed_cnt = 0;
			for (const auto& s : g_mainWindow->signals) {
				detected_in_window_cnt += s.GetDetectionsInWindow();
				detected_out_window_cnt += s.GetDetectionsOutWindow();
				signal_missed_cnt += s.GetMissed();
			}
			g_mainWindow->label_info_detected_in_window->SetText(std::to_string(detected_in_window_cnt));
			g_mainWindow->label_info_detected_out_window->SetText(std::to_string(detected_out_window_cnt));
			g_mainWindow->label_info_signal_missed->SetText(std::to_string(signal_missed_cnt));

			sf::sleep(sf::milliseconds(100));
		}
	}
}

static void GetData() {
	static float fbuf[N_CHANNELS * DATA_PER_CHANNEL];
	static int cntr = 0;
	bool running = true;

	while (g_mainWindow->IsOpen()) {

		if (g_communication->IsConnected() &&
			g_running == Running::RUNNING) {

			g_communication->Read(fbuf, 4);
			uint32_t delim = *((uint32_t*)&fbuf[0]);

			if (delim == 0xDEADBEEF) {	// Data
				int read = g_communication->Read(fbuf, sizeof(fbuf));
				if (read > 0) {
					float* fbuf_tmp = fbuf;
					for (auto& s : g_mainWindow->signals) {				
						s.Edit(fbuf_tmp, cntr * DATA_PER_CHANNEL, DATA_PER_CHANNEL);
						fbuf_tmp += DATA_PER_CHANNEL;
					}
					if (++cntr >= (g_n_samples / DATA_PER_CHANNEL)) {
						cntr = 0;
						if (g_record == Record::ALL) {
							for (auto const& s : g_mainWindow->signals)
								g_mainWindow->recorded_signals.push_back(s);
							g_mainWindow->label_recorded_signals_counter->SetText(std::to_string(g_mainWindow->recorded_signals.size() / N_CHANNELS));
						}
						else if (g_record == Record::EVENTS) {
							bool tmp_record = false;
							for (auto const& s : g_mainWindow->signals) {
								if (s.AnyEvents()) {
									tmp_record = true;
									break;
								}
							}
							
							if (tmp_record) {
								for (auto& s : g_mainWindow->signals) {
									if (s.AnyEvents()) {
										g_mainWindow->recorded_signals.push_back(s);
										s.ClearEvents();
									}
									else {
										g_mainWindow->recorded_signals.push_back(Signal()); // push empty signal
									}
								}
								g_mainWindow->label_recorded_signals_counter->SetText(std::to_string(g_mainWindow->recorded_signals.size() / N_CHANNELS));
							}							
						}
					}
				}
			}
			else if (delim == 0xABCDDCBA) {	// Sorting analysis
				int read = g_communication->Read(fbuf, ANALYSIS_PACKETS * N_CHANNELS * sizeof(uint32_t));
				g_analysisWindow->NewData((uint32_t*)fbuf, read / sizeof(uint32_t)); // "/sizeof(uint32_t)" because the data is 4 bytes not 1
			}
		}

	}	
}

void Application::InitFromFile(const std::string& file_name) {
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
			g_mainWindow->textbox_comport->SetText(tokens[i]);
			break;
		}
	}
}

void Application::Init() {
	g_communication = new Communication();
	g_mainWindow = new MainWindow(1850, 900, "Sorting Control", sf::Style::None | sf::Style::Close);	
	g_analysisWindow = new AnalysisWindow("Info");
	g_analysisWindow->SetPosition(g_mainWindow->GetPosition() + sf::Vector2i(1850-480, 40));
	g_analysisWindow->AlwaysOnTop(true);
	g_analysisWindow->MakeTransparent();
	g_analysisWindow->SetTransparency(150);

	// Initial parameters from file init
	InitFromFile("config.txt");

	g_running = Running::STOPPED;
	g_record = Record::NO;
	g_view = View::FILTERED;
	g_triggerframe = TriggerFrame::OFF;

	g_thread_info = std::thread(Information);
	g_thread_get_data = std::thread(GetData);
}

void Application::Run() {
	while (g_mainWindow->IsOpen()) {
		g_mainWindow->Update();
		g_analysisWindow->Update();		
	}

	g_thread_info.join();
	g_thread_get_data.join();

	delete g_analysisWindow;
	delete g_mainWindow;
	delete g_communication;
}
