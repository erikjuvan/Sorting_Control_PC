#include "MainWindow.hpp"
#include "AnalysisWindow.hpp"
#include "Communication.hpp"
#include "Helpers.hpp"
#include "Application.hpp"

extern Communication	*communication;
extern MainWindow		*mainWindow;
extern AnalysisWindow	*analysisWindow;

extern std::thread thread_info;
extern std::thread thread_get_data;

extern Running running;
extern Mode	mode;
extern View	view;
extern Capture capture;

extern int n_samples;

void MainWindow::button_connect_Click() {
	if (!communication->IsConnected()) {
		if (communication->Connect(mainWindow->textbox_comport->GetText())) {

			mainWindow->button_connect->SetText("Disconnect");
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
		mainWindow->button_connect->SetText("Connect");

		thread_info.detach();
		thread_get_data.detach();
	}
}

void MainWindow::button_run_Click() {
	if (running == Running::STOPPED) {
		running = Running::RUNNING;
		mainWindow->button_run->SetText("Running");

		for (int i = 0; i < N_CHANNELS; ++i)
			mainWindow->chart->ChangeSignal(i, &mainWindow->signals[i]);
	}
	else if (running == Running::RUNNING) {
		running = Running::STOPPED;
		mainWindow->button_run->SetText("Stopped");
	}
}

void MainWindow::button_toggle_usb_uart_Click() {
	static enum class Com { USB, UART } com{ Com::USB };

	if (com == Com::USB) {
		com = Com::UART;
		communication->Write("USBN\n");
		mainWindow->button_toggle_usb_uart->SetText("UART");
	}
	else if (com == Com::UART) {
		com = Com::USB;
		communication->Write("USBY\n");
		mainWindow->button_toggle_usb_uart->SetText("USB");
	}
}

void MainWindow::button_trigger_frame_Click() {
	static bool trigger_frame = false;
	if (trigger_frame == false) {
		trigger_frame = true;
		communication->Write("TRGFRM,1\n");
		mainWindow->chart->EnableTriggerFrame();
		mainWindow->button_trigger_frame->SetText("Frame ON");
	}
	else {
		trigger_frame = false;
		communication->Write("TRGFRM,0\n");
		mainWindow->chart->DisableTriggerFrame();
		mainWindow->button_trigger_frame->SetText("Frame OFF");
	}
}

void MainWindow::button_set_frequency_Click() {
	communication->Write("CSETF," + mainWindow->textbox_frequency->GetText() + "\n");
}

void MainWindow::button_set_filter_params_Click() {
	communication->Write("CPARAMS," + mainWindow->textbox_filter_params->GetText() + "\n");

	// Set threashold for all signals
	std::vector<std::string> strings = Help::TokenizeString(mainWindow->textbox_filter_params->GetText());
	for (auto& s : mainWindow->signals) {
		s.SetThreashold(std::stof(strings[3]));
	}
}

void MainWindow::button_set_times_Click() {
	communication->Write("CTIMES," + mainWindow->textbox_times->GetText() + "\n");

	// Set blind time for all signals
	std::vector<std::string> strings = Help::TokenizeString(mainWindow->textbox_times->GetText());
	for (auto& s : mainWindow->signals) {
		s.SetBlindTime(std::stoi(strings[2]));
	}
}

void MainWindow::button_view_mode_Click() {
	if (view == View::FILTERED) {
		view = View::RAW;
		communication->Write("CRAW\n");
		mainWindow->button_view_mode->SetText("Raw");
	}
	else if (view == View::RAW) {
		view = View::FILTERED;
		communication->Write("CFILTERED\n");
		mainWindow->button_view_mode->SetText("Filtered");
	}
}

void MainWindow::button_capture_Click() {
	if (capture == Capture::OFF) {
		capture = Capture::ON;
		mainWindow->button_capture->SetColor(sf::Color::Green);
	}
	else if (capture == Capture::ON) {
		capture = Capture::OFF;
		mainWindow->button_capture->ResetColor();
	}
}

void MainWindow::button_record_Click() {
	if (mode == Mode::LIVE) {
		mode = Mode::RECORD;
		mainWindow->button_record->SetColor(sf::Color::Red);
		mainWindow->recorded_signals.clear();
	}
	else if (mode == Mode::RECORD) {
		mode = Mode::LIVE;
		mainWindow->button_record->ResetColor();
	}
}

void MainWindow::button_analysis_window_Click() {
	if (!analysisWindow->IsOpen()) {
		analysisWindow->Create(390, 360, "Info", sf::Style::None | sf::Style::Close);
		analysisWindow->SetPosition(mainWindow->GetPosition() + sf::Vector2i(1850 - 420, 40));
		analysisWindow->AlwaysOnTop(true);
		analysisWindow->MakeTransparent();
		analysisWindow->SetTransparency(120);

	}	
}

void MainWindow::label_info_detected_in_window_Clicked() {
	for (auto& s : mainWindow->signals) {
		s.ClearDetectionsInWindow();
	}
}

void MainWindow::label_info_detected_out_window_Clicked() {

	for (auto& s : mainWindow->signals) {
		s.ClearDetectionsOutWindow();
	}
}

void MainWindow::label_info_signal_missed_Clicked() {
	for (auto& s : mainWindow->signals) {
		s.ClearMissed();
	}
}

void MainWindow::checkbox_only_show_framed_Clicked() {
	for (auto& s : mainWindow->signals) {
		s.OnlyDrawOnTrigger(mainWindow->checkbox_only_show_framed->IsChecked());
	}
}

void MainWindow::checkbox_transparent_Clicked() {
	static bool transparent = false;
	if (!transparent) {
		transparent = true;
		mainWindow->MakeTransparent();
		mainWindow->SetTransparency(120);
	}
	else {
		transparent = false;
		mainWindow->SetTransparency(255);
	}
}

void MainWindow::chart_OnKeyPress(const sf::Event& event) {
	static int frame_idx = -1;	// -1 so that when we first press right arrow we get the first [0] frame
	if (mode == Mode::RECORD && running == Running::STOPPED) {
		if (event.key.code == sf::Keyboard::Left) {
			if (frame_idx > 0) {
				frame_idx--;
				for (int i = 0; i < N_CHANNELS; ++i)
					mainWindow->chart->ChangeSignal(i, &mainWindow->recorded_signals[frame_idx * N_CHANNELS + i]);
			}
		}
		if (event.key.code == sf::Keyboard::Right) {
			const int size = ((int)mainWindow->recorded_signals.size() / N_CHANNELS);	// conversion from size_t to int (const int size) must be made or the bottom evaluation is wrong since comparing signed to unsigned, compiler promotes signed to unsigned converting -1 to maximum int value
			if (frame_idx < (size - 1)) {
				frame_idx++;
				for (int i = 0; i < N_CHANNELS; ++i)
					mainWindow->chart->ChangeSignal(i, &mainWindow->recorded_signals[frame_idx * N_CHANNELS + i]);
			}
		}
	}

	switch (event.key.code) {
	case sf::Keyboard::Num0:
		mainWindow->chart->ToggleDrawAllSignals();
		break;
	case sf::Keyboard::Num1:
		mainWindow->chart->ToggleDrawSignal(1);
		break;
	case sf::Keyboard::Num2:
		mainWindow->chart->ToggleDrawSignal(2);
		break;
	case sf::Keyboard::Num3:
		mainWindow->chart->ToggleDrawSignal(3);
		break;
	case sf::Keyboard::Num4:
		mainWindow->chart->ToggleDrawSignal(4);
		break;
	case sf::Keyboard::Num5:
		mainWindow->chart->ToggleDrawSignal(5);
		break;
	case sf::Keyboard::Num6:
		mainWindow->chart->ToggleDrawSignal(6);
		break;
	case sf::Keyboard::Num7:
		mainWindow->chart->ToggleDrawSignal(7);
		break;
	case sf::Keyboard::Num8:
		mainWindow->chart->ToggleDrawSignal(8);
		break;
	}
}

void MainWindow::CreateChart(int samples) {
	n_samples = samples;
	chart = new mygui::Chart(240, 10, 1600, 880, n_samples, 100);
	chart->CreateGrid(9);
	chart->OnKeyPress(&MainWindow::chart_OnKeyPress);
	signals.clear();
	signals.reserve(N_CHANNELS);
	for (int i = 0; i < N_CHANNELS; ++i) {
		signals.push_back(mygui::Signal(n_samples, sf::Color(m_Colors[i]), chart->GetGraphRegion(), chart->GetMaxVal()));
		chart->AddSignal(&signals[signals.size() - 1]);
	}
}

void MainWindow::RunClick() {
	button_run_Click();
}

MainWindow::MainWindow(int w, int h, const char* title, sf::Uint32 style) : Window(w, h, title, style) {
	///////////
	// Chart //
	///////////
	constexpr int N_SAMPLES = 10000;
	CreateChart(N_SAMPLES);

	/////////////
	// Buttons //
	/////////////
	button_connect = new mygui::Button(10, 50, "Connect", 110);
	button_connect->OnClick(&MainWindow::button_connect_Click);

	button_run = new mygui::Button(10, 90, "Stopped", 110);
	button_run->OnClick(&MainWindow::button_run_Click);

	button_trigger_frame = new mygui::Button(10, 140, "Frame OFF", 110);
	button_trigger_frame->OnClick(&MainWindow::button_trigger_frame_Click);

	button_capture = new mygui::Button(140, 140, "Capture");
	button_capture->OnClick(&MainWindow::button_capture_Click);

	button_toggle_usb_uart = new mygui::Button(140, 50, "USB");
	button_toggle_usb_uart->OnClick(&MainWindow::button_toggle_usb_uart_Click);

	button_view_mode = new mygui::Button(140, 90, "Filtered");
	button_view_mode->OnClick(&MainWindow::button_view_mode_Click);

	button_set_frequency = new mygui::Button(10, 260, "Send");
	button_set_frequency->OnClick(&MainWindow::button_set_frequency_Click);

	button_set_filter_params = new mygui::Button(10, 380, "Send");
	button_set_filter_params->OnClick(&MainWindow::button_set_filter_params_Click);

	button_set_times = new mygui::Button(10, 500, "Send");
	button_set_times->OnClick(&MainWindow::button_set_times_Click);

	button_record = new mygui::Button(10, 600, "Record");
	button_record->OnClick(&MainWindow::button_record_Click);

	button_analysis_window = new mygui::Button(10, 800, "Info");
	button_analysis_window->OnClick(&MainWindow::button_analysis_window_Click);

	//////////////
	// Texboxes //
	//////////////
	textbox_comport = new mygui::Textbox(10, 10, "COM", 80);
	textbox_frequency = new mygui::Textbox(10, 220, "10000", 80);
	textbox_filter_params = new mygui::Textbox(10, 340, "0.01,0.03,0.03,7.0", 170);
	textbox_times = new mygui::Textbox(10, 460, "0,100,1000", 120);

	////////////
	// Labels //
	////////////			
	label_frequency = new mygui::Label(10, 190, "Sample frequency:");
	label_filter_params = new mygui::Label(10, 310, "Filter params(a1,a2,a3,thr):");
	label_times = new mygui::Label(10, 430, "Times (dly, dur, blind):");
	label_info_rx_bytes = new mygui::Label(10, 660, "Rx buf: 0 bytes");
	label_info_detected_in_window = new mygui::Label(10, 700, "Det IN: 0");
	label_info_detected_in_window->OnClick(&MainWindow::label_info_detected_in_window_Clicked);
	label_info_detected_out_window = new mygui::Label(10, 720, "Det OUT: 0");
	label_info_detected_out_window->OnClick(&MainWindow::label_info_detected_out_window_Clicked);
	label_info_signal_missed = new mygui::Label(10, 740, "Missed: 0");
	label_info_signal_missed->OnClick(&MainWindow::label_info_signal_missed_Clicked);


	////////////////
	// Checkboxes //
	////////////////
	checkbox_only_show_framed = new mygui::Checkbox(10, 550, "Only show framed");
	checkbox_only_show_framed->OnClick(&MainWindow::checkbox_only_show_framed_Clicked);

	checkbox_transparent = new mygui::Checkbox(10, 850, "Transparent");
	checkbox_transparent->OnClick(&MainWindow::checkbox_transparent_Clicked);


	/////////////////
	// Main window //
	/////////////////
	Add(chart);
	// Buttons
	Add(button_connect);
	Add(button_run);
	Add(button_trigger_frame);
	Add(button_set_frequency);
	Add(button_set_filter_params);
	Add(button_set_times);
	Add(button_toggle_usb_uart);
	Add(button_view_mode);
	Add(button_capture);
	Add(button_record);
	Add(button_analysis_window);
	// Texboxes
	Add(textbox_comport);
	Add(textbox_frequency);
	Add(textbox_filter_params);
	Add(textbox_times);
	// Labels		
	Add(label_frequency);
	Add(label_filter_params);
	Add(label_times);
	Add(label_info_rx_bytes);
	Add(label_info_detected_in_window);
	Add(label_info_detected_out_window);
	Add(label_info_signal_missed);
	// Checkboxes
	Add(checkbox_only_show_framed);
	Add(checkbox_transparent);
}

MainWindow::~MainWindow() {

}