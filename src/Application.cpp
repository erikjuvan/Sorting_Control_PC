#include "Application.hpp"
#include "MainWindow.hpp"
#include "AnalysisWindow.hpp"
#include "Communication.hpp"
#include <fstream>
#include <thread>

Communication	*communication;
MainWindow		*mainWindow;
AnalysisWindow	*analysisWindow;

Running running;
Mode	mode;
View	view;
Capture capture;

std::thread thread_info;
std::thread thread_get_data;

int n_samples;

void Information() {
	static int cnt = 0;
	static int detected_in_window_cnt = 0, detected_out_window_cnt = 0, signal_missed_cnt = 0;

	while (communication->IsConnected()) {
		mainWindow->label_info_rx_bytes->SetText(std::to_string(cnt++) + " Rx buf: " + std::to_string(communication->GetRxBufferLen()) + " bytes");

		detected_in_window_cnt = detected_out_window_cnt = signal_missed_cnt = 0;
		for (const auto& s : mainWindow->signals) {
			detected_in_window_cnt += s.GetDetectionsInWindow();
			detected_out_window_cnt += s.GetDetectionsOutWindow();
			signal_missed_cnt += s.GetMissed();
		}
		mainWindow->label_info_detected_in_window->SetText("Det IN: " + std::to_string(detected_in_window_cnt));
		mainWindow->label_info_detected_out_window->SetText("Det OUT: " + std::to_string(detected_out_window_cnt));
		mainWindow->label_info_signal_missed->SetText("Missed: " + std::to_string(signal_missed_cnt));

		sf::sleep(sf::milliseconds(100));
	}
}

void GetData() {
	static float fbuf[N_CHANNELS * DATA_PER_CHANNEL];
	static int cntr = 0;
	static bool thr_missed;

	while (communication->IsConnected()) {

		if (running == Running::RUNNING) {	

			communication->Read(fbuf, 4);
			uint32_t delim = *((uint32_t*)&fbuf[0]);

			if (delim == 0xDEADBEEF) {	// Data
				int read = communication->Read(fbuf, sizeof(fbuf));
				if (read > 0) {
					thr_missed = false;
					float* fbuf_tmp = fbuf;
					for (int ch = 0; ch < N_CHANNELS; ++ch) {
						mainWindow->signals[ch].Edit(fbuf_tmp, cntr * DATA_PER_CHANNEL, DATA_PER_CHANNEL);
						if (mainWindow->signals[ch].ThreasholdMissed() && capture == Capture::ON && !thr_missed) {
							mainWindow->RunClick();	// only change state here
							thr_missed = true;
							// we don't break out, because we want to draw out the remaining buffer
						}

						fbuf_tmp += DATA_PER_CHANNEL;
					}
					if (++cntr >= (n_samples / DATA_PER_CHANNEL)) {
						cntr = 0;
						if (mode == Mode::RECORD) {
							for (const auto& s : mainWindow->signals)
								mainWindow->recorded_signals.push_back(s);
						}
					}
				}
			}
			else if (delim == 0xABCDDCBA) {	// Sorting analysis
				int read = communication->Read(fbuf, ANALYSIS_PACKETS * N_CHANNELS * sizeof(uint32_t));
				analysisWindow->NewData((uint32_t*)fbuf, read / sizeof(uint32_t)); // "/2" because the data is int16 not int8
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
			mainWindow->textbox_comport->SetText(tokens[i]);
			break;
		}
	}
}

void Application::Init() {
	communication = new Communication();
	mainWindow = new MainWindow(1850, 900, "Sorting Control", sf::Style::None | sf::Style::Close);	
	analysisWindow = new AnalysisWindow(390, 360, "Info", sf::Style::None | sf::Style::Close);
	analysisWindow->SetPosition(mainWindow->GetPosition() + sf::Vector2i(1850-420, 40));
	analysisWindow->AlwaysOnTop(true);
	analysisWindow->MakeTransparent();
	analysisWindow->SetTransparency(120);

	// Initial parameters from file init
	InitFromFile("config.txt");

	running = Running::STOPPED;
	mode = Mode::LIVE;
	view = View::FILTERED;
	capture = Capture::OFF;
}

void Application::Run() {
	while (mainWindow->IsOpen()) {
		mainWindow->Update();
		analysisWindow->Update();		
	}
}
