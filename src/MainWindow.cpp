#include "MainWindow.hpp"
#include "AnalysisWindow.hpp"
#include "Helpers.hpp"

void MainWindow::button_connect_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	if (!obj.m_application->communication->IsConnected()) {
		if (obj.m_application->communication->Connect(obj.textbox_comport->GetText())) {

			obj.button_connect->SetText("Disconnect");
			obj.m_application->communication->Write("USBY\n");
			button_set_frequency_Click(vobj);
			button_set_filter_params_Click(vobj);
			button_set_times_Click(vobj);

			obj.m_application->communication->Write("CFILTERED\n");
			obj.m_application->communication->Write("VRBS,1\n");

			obj.m_application->thread_info = std::thread(&Application::Information, obj.m_application);
			obj.m_application->thread_get_data = std::thread(&Application::GetData, obj.m_application);
		}
	}
	else {
		obj.m_application->communication->Write("USBY\n");
		obj.m_application->communication->Disconnect();
		obj.button_connect->SetText("Connect");

		obj.m_application->thread_info.detach();
		obj.m_application->thread_get_data.detach();
	}
}

void MainWindow::button_run_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	if (obj.m_application->m_running == Application::Running::STOPPED) {
		obj.m_application->m_running = Application::Running::RUNNING;
		obj.button_run->SetText("Running");

		for (int i = 0; i < Application::N_CHANNELS; ++i)
			obj.chart->ChangeSignal(i, &obj.signals[i]);
	}
	else if (obj.m_application->m_running == Application::Running::RUNNING) {
		obj.m_application->m_running = Application::Running::STOPPED;
		obj.button_run->SetText("Stopped");
	}
}

void MainWindow::button_toggle_usb_uart_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	static enum class Com { USB, UART } com{ Com::USB };

	if (com == Com::USB) {
		com = Com::UART;
		obj.m_application->communication->Write("USBN\n");
		obj.button_toggle_usb_uart->SetText("UART");
	}
	else if (com == Com::UART) {
		com = Com::USB;
		obj.m_application->communication->Write("USBY\n");
		obj.button_toggle_usb_uart->SetText("USB");
	}
}

void MainWindow::button_trigger_frame_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	static bool trigger_frame = false;
	if (trigger_frame == false) {
		trigger_frame = true;
		obj.m_application->communication->Write("TRGFRM,1\n");
		obj.chart->EnableTriggerFrame();
		obj.button_trigger_frame->SetText("Frame ON");
	}
	else {
		trigger_frame = false;
		obj.m_application->communication->Write("TRGFRM,0\n");
		obj.chart->DisableTriggerFrame();
		obj.button_trigger_frame->SetText("Frame OFF");
	}
}

void MainWindow::button_set_frequency_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	obj.m_application->communication->Write("CSETF," + obj.textbox_frequency->GetText() + "\n");
}

void MainWindow::button_set_filter_params_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	obj.m_application->communication->Write("CPARAMS," + obj.textbox_filter_params->GetText() + "\n");

	// Set threashold for all signals
	std::vector<std::string> strings = Help::TokenizeString(obj.textbox_filter_params->GetText());
	for (auto& s : obj.signals) {
		s.SetThreashold(std::stof(strings[3]));
	}
}

void MainWindow::button_set_times_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	obj.m_application->communication->Write("CTIMES," + obj.textbox_times->GetText() + "\n");

	// Set blind time for all signals
	std::vector<std::string> strings = Help::TokenizeString(obj.textbox_times->GetText());
	for (auto& s : obj.signals) {
		s.SetBlindTime(std::stoi(strings[2]));
	}
}

void MainWindow::button_view_mode_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	if (obj.m_application->m_view == Application::View::FILTERED) {
		obj.m_application->m_view = Application::View::RAW;
		obj.m_application->communication->Write("CRAW\n");
		obj.button_view_mode->SetText("Raw");
	}
	else if (obj.m_application->m_view == Application::View::RAW) {
		obj.m_application->m_view = Application::View::FILTERED;
		obj.m_application->communication->Write("CFILTERED\n");
		obj.button_view_mode->SetText("Filtered");
	}
}

void MainWindow::button_capture_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	if (obj.m_application->m_capture == Application::Capture::OFF) {
		obj.m_application->m_capture = Application::Capture::ON;
		obj.button_capture->SetColor(sf::Color::Green);
	}
	else if (obj.m_application->m_capture == Application::Capture::ON) {
		obj.m_application->m_capture = Application::Capture::OFF;
		obj.button_capture->ResetColor();
	}
}

void MainWindow::button_record_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	if (obj.m_application->m_mode == Application::Mode::LIVE) {
		obj.m_application->m_mode = Application::Mode::RECORD;
		obj.button_record->SetColor(sf::Color::Red);
		obj.recorded_signals.clear();
	}
	else if (obj.m_application->m_mode == Application::Mode::RECORD) {
		obj.m_application->m_mode = Application::Mode::LIVE;
		obj.button_record->ResetColor();
	}
}

void MainWindow::button_analysis_window_Click(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	static const bool show = false;
	if (show)
		obj.m_application->analysisWindow->Hide();
	else	
		obj.m_application->analysisWindow->Show();
}

void MainWindow::checkbox_only_show_framed_Clicked(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	for (auto& s : obj.signals) {
		s.OnlyDrawOnTrigger(obj.checkbox_only_show_framed->IsChecked());
	}
}

void MainWindow::label_info_detected_in_window_Clicked(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	for (auto& s : obj.signals) {
		s.ClearDetectionsInWindow();
	}
}

void MainWindow::label_info_detected_out_window_Clicked(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	for (auto& s : obj.signals) {
		s.ClearDetectionsOutWindow();
	}
}

void MainWindow::label_info_signal_missed_Clicked(void* vobj) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	for (auto& s : obj.signals) {
		s.ClearMissed();
	}
}

void MainWindow::chart_OnKeyPress(void* vobj, const sf::Event& event) {
	MainWindow& obj = *static_cast<MainWindow*>(vobj);
	static int frame_idx = -1;	// -1 so that when we first press right arrow we get the first [0] frame
	if (obj.m_application->m_mode == Application::Mode::RECORD && obj.m_application->m_running == Application::Running::STOPPED) {
		if (event.key.code == sf::Keyboard::Left) {
			if (frame_idx > 0) {
				frame_idx--;
				for (int i = 0; i < Application::N_CHANNELS; ++i)
					obj.chart->ChangeSignal(i, &obj.recorded_signals[frame_idx * Application::N_CHANNELS + i]);
			}
		}
		if (event.key.code == sf::Keyboard::Right) {
			const int size = ((int)obj.recorded_signals.size() / Application::N_CHANNELS);	// conversion from size_t to int (const int size) must be made or the bottom evaluation is wrong since comparing signed to unsigned, compiler promotes signed to unsigned converting -1 to maximum int value
			if (frame_idx < (size - 1)) {
				frame_idx++;
				for (int i = 0; i < Application::N_CHANNELS; ++i)
					obj.chart->ChangeSignal(i, &obj.recorded_signals[frame_idx * Application::N_CHANNELS + i]);
			}
		}
	}

	switch (event.key.code) {
	case sf::Keyboard::Num0:
		obj.chart->ToggleDrawAllSignals();
		break;
	case sf::Keyboard::Num1:
		obj.chart->ToggleDrawSignal(1);
		break;
	case sf::Keyboard::Num2:
		obj.chart->ToggleDrawSignal(2);
		break;
	case sf::Keyboard::Num3:
		obj.chart->ToggleDrawSignal(3);
		break;
	case sf::Keyboard::Num4:
		obj.chart->ToggleDrawSignal(4);
		break;
	case sf::Keyboard::Num5:
		obj.chart->ToggleDrawSignal(5);
		break;
	case sf::Keyboard::Num6:
		obj.chart->ToggleDrawSignal(6);
		break;
	case sf::Keyboard::Num7:
		obj.chart->ToggleDrawSignal(7);
		break;
	case sf::Keyboard::Num8:
		obj.chart->ToggleDrawSignal(8);
		break;
	}
}

void MainWindow::CreateChart(int n_samples) {
	m_application->m_n_samples = n_samples;
	chart = new mygui::Chart(240, 10, 1600, 880, m_application->m_n_samples, 100);
	chart->CreateGrid(9);
	chart->OnKeyPress(this, &MainWindow::chart_OnKeyPress);
	signals.clear();
	signals.reserve(Application::N_CHANNELS);
	for (int i = 0; i < Application::N_CHANNELS; ++i) {
		signals.push_back(mygui::Signal(m_application->m_n_samples, sf::Color(m_Colors[i]), chart->GetGraphRegion(), chart->GetMaxVal()));
		chart->AddSignal(&signals[signals.size() - 1]);
	}
}

void MainWindow::RunClick() {
	button_run_Click(this);
}

bool MainWindow::IsOpen() {
	return m_window->IsOpen();
}

MainWindow::MainWindow(int w, int h, const char* title, Application* application) : m_application(application) {
	///////////
	// Chart //
	///////////
	constexpr int N_SAMPLES = 10000;
	CreateChart(N_SAMPLES);

	/////////////
	// Buttons //
	/////////////
	button_connect = new mygui::Button(10, 50, "Connect", 110);
	button_connect->OnClick(this, &MainWindow::button_connect_Click);

	button_run = new mygui::Button(10, 90, "Stopped", 110);
	button_run->OnClick(this, &MainWindow::button_run_Click);

	button_trigger_frame = new mygui::Button(10, 140, "Frame OFF", 110);
	button_trigger_frame->OnClick(this, &MainWindow::button_trigger_frame_Click);

	button_capture = new mygui::Button(140, 140, "Capture");
	button_capture->OnClick(this, &MainWindow::button_capture_Click);

	button_toggle_usb_uart = new mygui::Button(140, 50, "USB");
	button_toggle_usb_uart->OnClick(this, &MainWindow::button_toggle_usb_uart_Click);

	button_view_mode = new mygui::Button(140, 90, "Filtered");
	button_view_mode->OnClick(this, &MainWindow::button_view_mode_Click);

	button_set_frequency = new mygui::Button(10, 260, "Send");
	button_set_frequency->OnClick(this, &MainWindow::button_set_frequency_Click);

	button_set_filter_params = new mygui::Button(10, 380, "Send");
	button_set_filter_params->OnClick(this, &MainWindow::button_set_filter_params_Click);

	button_set_times = new mygui::Button(10, 500, "Send");
	button_set_times->OnClick(this, &MainWindow::button_set_times_Click);

	button_record = new mygui::Button(10, 600, "Record");
	button_record->OnClick(this, &MainWindow::button_record_Click);

	button_analysis_window = new mygui::Button(10, 800, "Info");
	button_analysis_window->OnClick(this, &MainWindow::button_analysis_window_Click);

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
	label_info_detected_in_window->OnClick(this, &MainWindow::label_info_detected_in_window_Clicked);
	label_info_detected_out_window = new mygui::Label(10, 720, "Det OUT: 0");
	label_info_detected_out_window->OnClick(this, &MainWindow::label_info_detected_out_window_Clicked);
	label_info_signal_missed = new mygui::Label(10, 740, "Missed: 0");
	label_info_signal_missed->OnClick(this, &MainWindow::label_info_signal_missed_Clicked);


	////////////////
	// Checkboxes //
	////////////////
	checkbox_only_show_framed = new mygui::Checkbox(10, 550, "Only show framed");
	checkbox_only_show_framed->OnClick(this, &MainWindow::checkbox_only_show_framed_Clicked);

	/////////////////
	// Main window //
	/////////////////
	m_window = new Window(w, h, title);
	m_window->Attach(chart);
	// Buttons
	m_window->Attach(button_connect);
	m_window->Attach(button_run);
	m_window->Attach(button_trigger_frame);
	m_window->Attach(button_set_frequency);
	m_window->Attach(button_set_filter_params);
	m_window->Attach(button_set_times);
	m_window->Attach(button_toggle_usb_uart);
	m_window->Attach(button_view_mode);
	m_window->Attach(button_capture);
	m_window->Attach(button_record);
	m_window->Attach(button_analysis_window);
	// Texboxes
	m_window->Attach(textbox_comport);
	m_window->Attach(textbox_frequency);
	m_window->Attach(textbox_filter_params);
	m_window->Attach(textbox_times);
	// Labels		
	m_window->Attach(label_frequency);
	m_window->Attach(label_filter_params);
	m_window->Attach(label_times);
	m_window->Attach(label_info_rx_bytes);
	m_window->Attach(label_info_detected_in_window);
	m_window->Attach(label_info_detected_out_window);
	m_window->Attach(label_info_signal_missed);
	// Checkboxes
	m_window->Attach(checkbox_only_show_framed);
}

MainWindow::~MainWindow() {

}